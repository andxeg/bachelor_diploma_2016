#include "prototype.h"

#include "preprocessor.h"
#include "request.h"
#include "element.h"
#include "network.h"
#include "criteria.h"
#include "operation.h"
#include "link.h"
#include "leafnode.h"
#include "routing/bfsrouter.h"
#include "exhaustivesearcher.h"
#include "dcoverseer.h"

#include "routing/bsearcher.h"
#include "routing/dijkstrarouter.h"
#include <stdio.h>

#include <vector>
#include <algorithm>
#include <queue>
#include <deque>
#include <map>

//
#include <string>
//

#define EPS 0.000001


//bool PrototypeAlgorithm::linkChange = false;


void PrototypeAlgorithm::schedule() {
    size_t assignedRequests = 0;
    std::vector<Request *> pRequests = prioritizeRequests(requests);
    // TODO: Make rack tagging
    // TODO: Create static DCOverseer with tag information
    for (std::vector<Request *>::iterator i = pRequests.begin();
            i != pRequests.end(); i++) 
    {
        Request * r = *i;
        Request * fakeRequest = Preprocessor::fakeNetElements(r);
        std::cout << r->getName().c_str() << std::endl;

        if ( scheduleRequest(fakeRequest)) {
            assignedRequests++;
            std::cout << "Now assigned -> " << assignedRequests << std::endl;
        } else {
            fprintf(stderr, "[ERROR] Failed to assign request %s.\n", r->getName().c_str());
            r->purgeAssignments();
            std::cout << "After r->purgeAssignments();"  << std::endl;
        }
        delete fakeRequest;   
    }
}

std::vector<Request *> PrototypeAlgorithm::prioritizeRequests(Requests & r) {
    std::vector<Request *> res;
    res.insert(res.end(), r.begin(), r.end());
    std::sort(res.begin(), res.end(), simpleIncreasing);
    return res;
}

bool PrototypeAlgorithm::simpleIncreasing(Request * first, Request * second) {
    return first->getElements().size() > second->getElements().size();
}  


bool PrototypeAlgorithm::scheduleRequest(Request * r) {
//#ifdef DL_AFFINE
//    if ( r->isDCAffined() ) {
//        if ( !dlRequestAssignment(r) ) {
//            fprintf(stderr, "[ERROR] tenant affinity requirement failed\n");
//            return false;
//        }
//    }
//#endif
    Elements pool = network->getNodes();

    //TODO: make tenant tags (DC)

//    Elements dcLayered = Operation::filter(r->elementsToAssign(), Criteria::isDCLayered);
//#ifndef DL_STRICT
//    if ( !dlAssignment(dcLayered, pool, r) ) {
//#else
//    if ( !dlAssignmentStrict(dcLayered, pool, r) ) {
//#endif
//        fprintf(stderr, "[ERROR] dc layer requirement failed\n");
//        return false;
//    }

    Elements serverLayered = Operation::filter(r->elementsToAssign(), Criteria::isServerLayered);
    if ( !slAssignment(serverLayered, pool, r) ) {
        fprintf(stderr, "[ERROR] server layer requirement failed\n");
        return false;
    }


    Elements unassignedNodes = Operation::filter(r->elementsToAssign(), Criteria::isComputational);
    return routedAssignment(unassignedNodes, pool, r);
}

bool PrototypeAlgorithm::dlRequestAssignment(Request * r) {
    Elements elements = r->elementsToAssign();
    Elements nodes = Operation::filter(elements, Criteria::isNode);
    Elements sle = Operation::filter(nodes, Criteria::isServerLayered);
    Elements others = Operation::minus(nodes, sle);

    DCOverseer overseer(network->getNodes()); 

    for( int i = 0; i < overseer.dcCount(); i++) {
        Elements pool = overseer.dcPositionPool(i);

        if ( !sle.empty() && !slAssignment(sle, pool, r) )
        {
            Operation::forEach(sle, Operation::unassign);
            continue;
        }

        if ( !others.empty() && !routedAssignment(others, pool, r) )
        {
            Operation::forEach(others, Operation::unassign);
            continue;
        }

        return true;

    }

    return false;

}

bool PrototypeAlgorithm::routedAssignment(Elements & n, Elements & pool, Request * r)
{
    //TODO: check for link assignment consumptions
    Elements nodes = Operation::filter(n, Criteria::isUnassigned);
    while ( !nodes.empty() ) {
        Element * unassignedSeed = getSeedElement(nodes);
        std::deque<Element *> queue;
        //tweakQueue(queue, r);
        queue.push_back(unassignedSeed);
        while ( !queue.empty() ) {
            Element * nextToAssign = queue.front();
            queue.pop_front();
            
            if ( nextToAssign->isAssigned() )
                continue;

            if ( nextToAssign->isSwitch() && !nextToAssign->isRouter())
                continue;

            BFSRouter router(*r, nextToAssign);
            bool result = false;

            if ( router.isValid() ) {
                result = router.search();
            } else {
                result = assignSeedElement(nextToAssign, pool);
            }

            if ( !result ) {
                Elements assignedElements = Elements();
                if ( !exhaustiveSearch(nextToAssign, pool, assignedElements) ) {
                    return false;
                }
            }

            Elements adjacentNodes = Operation::filter(nextToAssign->adjacentNodes(), Criteria::isUnassigned);
            Elements adjacentEdges = nextToAssign->adjacentEdges();
            std::vector<Element * > tmp;
            tmp.insert(tmp.end(), adjacentEdges.begin(), adjacentEdges.end());
            std::sort(tmp.begin(), tmp.end(), Criteria::elementWeightDescending);

            for(size_t e = 0; e < tmp.size(); e ++) {
                Edge * edge = tmp[e]->toEdge();
                for (Elements::iterator i = adjacentNodes.begin(); i != adjacentNodes.end(); i++){
                    if (edge->connects(*i)) {
                        queue.push_back(*i);
                        adjacentNodes.erase(*i);
                        break;
                    }
                }
            }

//            Elements adjacentNodes = nextToAssign->adjacentNodes();
//            for ( Elements::iterator i = adjacentNodes.begin(); i != adjacentNodes.end(); i++) {
//                queue.push_back(*i);
//            }
            nodes.erase(nextToAssign);
        }
    }

    return true;

}

bool PrototypeAlgorithm::slAssignment(Elements & nodes, Elements & pool, Request * r) {
    Elements layered = Operation::filter(nodes, Criteria::isServerLayered);

    if ( layered.empty() )
        return true;
    
    using std::map;
    map<int, Elements> layeredModel;
    for (Elements::iterator i = layered.begin(); i != layered.end(); i++) {
        LeafNode * l = (LeafNode *)(*i);
        layeredModel[l->sl()].insert(l);
    }


    for(map<int, Elements>::iterator i = layeredModel.begin(); i != layeredModel.end(); i++) {
        Elements & elements = i->second;

        //sort element by value
        std::vector<Element *> tmp = std::vector<Element *>(elements.begin(), elements.end());
        for (Elements::iterator j = elements.begin(); j != elements.end(); j++) {
            Element * elem = (*j);
            elem->value = virtualElementValue(elem);
        }
        std::sort(tmp.begin(), tmp.end(), descendingVirt);

//        std::sort(tmp.begin(), tmp.end(), Criteria::elementWeightDescending);
        //

        Elements globalCandidates = Operation::filter(pool, tmp[0], Criteria::canHostAssignment);
        std::vector<Element *> candidates = std::vector<Element *>(globalCandidates.begin(), globalCandidates.end());

        //sort physical elements by value
//        for (size_t i = 0; i < candidates.size(); i++) {
//            Element * elem = candidates[i];
//            elem->value = physicalElementValue(candidates[i], network);
//        }

//        std::sort(candidates.begin(), candidates.end(), descendingPhys);
            std::sort(candidates.begin(), candidates.end(), Criteria::elementWeightDescending);
        //

        for (std::vector<Element *>::iterator c = candidates.begin(); c != candidates.end(); c++ ) {
            Elements localAssigned;
            for ( Elements::iterator a = (*c)->getAssignments().begin(); a != (*c)->getAssignments().end(); a++ ) {
                if ( Operation::isIn(*a, r->getElements()) )
                    localAssigned.insert(*a);
            }

            bool layerConflict = false;
            for ( Elements::iterator a = localAssigned.begin(); a != localAssigned.end(); a++ ) {
                if ( !Criteria::isServerLayered(*a) )
                    continue;

                LeafNode * l = (LeafNode *)(*a);
                if ( l->sl() == i->first )
                    layerConflict = true;
            }
            if ( layerConflict )
                continue;

            bool result = true;
            for ( Elements::iterator e = elements.begin(); e != elements.end(); e++) {
                result &= (*c)->assign(*e);    
            }

            if ( result )
                break;

            Operation::forEach(elements, Operation::unassign);
        }
        if ( !Operation::filter(elements, Criteria::isUnassigned).empty() )
            return false;
    }

    return true; 
}

bool PrototypeAlgorithm::dlAssignment(Elements & nodes, Elements & pool, Request * r) {
    if ( nodes.empty() )
        return true;

    DCOverseer virtualOverseer(nodes);
    DCOverseer physicalOverseer(pool);

    if ( virtualOverseer.dcCount() > physicalOverseer.dcCount() ) {
        fprintf(stderr, "[ERROR] not enough physical datacenters to satisfy restrictions\n");
        return false;
    }

    using std::map;
    map<int, int> physicalAffinity;
    for(int i = 0; i < virtualOverseer.dcCount(); i++ ) {
        Elements elements = virtualOverseer.dcPositionPool(i);
        for ( int j = 0; j < physicalOverseer.dcCount(); j++ ) {
            if ( physicalAffinity.find(j) != physicalAffinity.end() )
                continue;

            Elements pool = physicalOverseer.dcPositionPool(j);
            if ( !slrAssignment(elements,pool, r))
                continue;

            physicalAffinity[j] = i;
            break;
        }        
    }

    if ( physicalAffinity.size() < virtualOverseer.dcCount() )
        return false;

    return true;
}

bool PrototypeAlgorithm::dlAssignmentStrict(Elements & nodes, Elements & pool, Request * r) {
    if ( nodes.empty() )
       return true;

    DCOverseer vo(nodes);
    DCOverseer po(pool);

    if ( vo.dcCount() > po.dcCount() ) {
        fprintf(stderr, "[ERROR] not enough physical datacenters to satisfy restrictions\n");
        return false;
    } 

    for ( int i = 0; i < vo.dcCount(); i++ ) {
        Elements elements = vo.dcPositionPool(i);
        printf("Laying out virtual dc %d into physical dc %d.\n", vo.dcPoolId(i), po.dcPoolId(i));
        Elements pool = po.dcPool(vo.dcPoolId(i));
        if ( !slrAssignment(elements, pool, r) ) {
            fprintf(stderr, "[ERROR] was unable to lay out dc with dl=%d\n", vo.dcPoolId(i));
            return false;
        }
    }

    return true;
}

bool PrototypeAlgorithm::slrAssignment(Elements & nodes, Elements & pool, Request * r) {
    Elements n = Operation::filter(nodes, Criteria::isUnassigned);
    Elements sle = Operation::filter(n, Criteria::isServerLayered);
    Elements other = Operation::minus(n, sle);
    if ( !sle.empty() && !slAssignment(sle, pool, r) ) {
        Operation::forEach(sle, Operation::unassign);
        return false;
    } 

    if ( !other.empty() && !routedAssignment(other, pool, r) ) {
        Operation::forEach(n, Operation::unassign);
        return false;
    }

    return true;

}

bool PrototypeAlgorithm::exhaustiveSearch(Element * e, Elements & pool,
                                          Elements & assignedElements, const bool & twoStage) {
    ExhaustiveSearcher searcher(network, resources, tenantsElements, pool, e, assignedElements, twoStage, 3);
    return searcher.search();
}

bool PrototypeAlgorithm::assignSeedElement(Element * e, Elements & pool) {
    Elements candidates = Operation::filter(pool, e, Criteria::canHostAssignment);
    if ( candidates.empty() )
        return false;

    Element * candidate = getSeedElement(candidates, false);
    return candidate->assign(e);
}

Elements PrototypeAlgorithm::connectedComponent(Element * e) {
    Elements result;

    if ( e->isLink() )
        e = e->toLink()->getFirst()->getParentNode();

    std::queue<Element *> queue;
    queue.push(e);
    while ( !queue.empty() ) {
        Element * node = queue.front();
        queue.pop();
        result.insert(node);

        Elements adjacentNodes = node->adjacentNodes();
        for ( Elements::iterator i = adjacentNodes.begin(); i != adjacentNodes.end(); i++ ) {
            Element * a = *i;
            if ( !Operation::isIn(a, result) ) {
                queue.push(a);
            }
        }
    }

    return result;
}

void PrototypeAlgorithm::tweakQueue(std::deque<Element *> & queue, Request * r) {
    Elements tunnels = r->getTunnels();
    std::vector<Element *> links;
    links.insert(links.end(), tunnels.begin(), tunnels.end());
    std::sort(links.begin(), links.end(), Criteria::elementWeightDescending);
    for (size_t i = 0; i < links.size(); i++) {
        Link * link = links[i]->toLink();
        if ( link->isAffine() ) {
            Element * first = link->getFirst()->getParentNode();
            Element * second = link->getSecond()->getParentNode();
            Element * vm, * store;
            if ( first->isComputer() ) {
                vm = first;
                store = second;
            } else {
                store = first;
                vm = second;
            }
            queue.push_back(store);
            queue.push_back(vm);
        }
    }
}

Element * PrototypeAlgorithm::getSeedElement(Elements & e, bool isVirtual) {
    std::vector<Element *> tmp;
    Elements stores = Operation::filter(e, Criteria::isStore);
    if ( !stores.empty() ) {
        tmp = std::vector<Element *>(stores.begin(), stores.end());
    } else {
        tmp = std::vector<Element *>(e.begin(), e.end());
    }

    if ( isVirtual ) {


        for (std::vector<Element *>::iterator i = tmp.begin(); i != tmp.end(); i++) {
            Element * elem = (*i);
            elem->value = virtualElementValue(elem);
        }


        std::sort(tmp.begin(), tmp.end(), descendingVirt);
//        std::sort(tmp.begin(), tmp.end(), Criteria::elementWeightDescending);
    } else {
        for (size_t i = 0; i < tmp.size(); i++) {
            Element * elem = tmp[i];
            elem->value = physicalElementValue(tmp[i], network);
        }

        //PrototypeAlgorithm::linkChange = false;

        std::sort(tmp.begin(), tmp.end(), ascendingPhys);
//        std::sort(tmp.begin(), tmp.end(), Criteria::elementWeightAscending);
    }
    Element * result = tmp[0];
    return result;
} 


bool PrototypeAlgorithm::descendingVirt(const Element * first, const Element * second) {
    return first->value > second->value;
}


bool PrototypeAlgorithm::ascendingPhys(const Element * first, const Element * second) {
    return first->value < second->value;
}

bool PrototypeAlgorithm::descendingPhys(const Element *first, const Element *second) {
    return first->value > second->value;
}

double PrototypeAlgorithm::virtualElementValue(Element * element) {
    double result = 0.0;
    Elements edges = element->adjacentEdges();
    for (Elements::iterator i = edges.begin(); i != edges.end(); i++) {
        result += (*i)->toLink()->getFullThroughput();
    }

    return result * element->weight();
}

double PrototypeAlgorithm::physicalElementValue(Element * element, Network * network) {
    double result = 0.0;
    Elements edges = element->adjacentEdges();
    for (Elements::iterator i = edges.begin(); i != edges.end(); i++) {
        //result += (*i)->toLink()->getFullThroughput();
        result += (*i)->toLink()->getThroughput();
    }

    if (result < EPS)
        return result;

    result *= element->weight();
    double closeness = 0.0;

//    if (PrototypeAlgorithm::linkChange == false) {
//        std::map<Element *, double>::iterator i;
//        for(i = element->reachableElements.begin();i != element->reachableElements.end(); i++) {
//            closeness += i->first->weight() * i->second;
//        }
//    } else {



        //if links were changed then first delete old information about reachableElements
//        element->reachableElements.clear();

    //find shortest pathes from current node to all
    //path consist of only links
    //if path is not exist from <current> element to <another> element then pathes
    //hasn't key <another>
    DijkstraRouter router = DijkstraRouter(const_cast<Element *>(element), network);
    router.route();
    std::map<Element *, std::vector<Element * > > pathes = router.getPathes();
    std::map<Element *, std::vector<Element * > >::iterator i;
    for (i = pathes.begin(); i != pathes.end(); i++) {
        Element * neighbor = i->first;
        std::vector<Element * > path = i->second;
        uint minThroughput = path[0]->toLink()->getThroughput();
        uint currThroughput;

        for (size_t j = 0; j < path.size(); j++) {
            currThroughput = path[j]->toLink()->getThroughput();
            if (currThroughput < minThroughput)
                minThroughput = currThroughput;
        }

//            element->reachableElements[neighbor] =  (double(minThroughput) / path.size());
        closeness += neighbor->weight() * ( double( minThroughput ) / path.size());
    }
//    }

    if (closeness < EPS)
        return result;

    return result * closeness;
}

//=========================NEW_ASSIGNMENT_STRATEGY=============================
//============================TWO_STAGE_ALGORITHM==============================

void PrototypeAlgorithm::twoStageSchedule() {
    size_t assignedRequests = 0;
    std::vector<Request *> pRequests = prioritizeRequests(requests);
    for (std::vector<Request *>::iterator i = pRequests.begin();
         i != pRequests.end(); i++)
    {
        Request * r = *i;
        Request * fakeRequest = Preprocessor::fakeNetElements(r);

        std::cout << r->getName().c_str() << std::endl;

        if ( assignNodesAndLinks(fakeRequest)) {
            assignedRequests++;
            std::cout << "Now assigned -> " << assignedRequests << std::endl;
        } else {
            fprintf(stderr, "[ERROR] Failed to assign request %s.\n", r->getName().c_str());
            r->purgeAssignments();
            std::cout << "After r->purgeAssignments();"  << std::endl;
        }
        delete fakeRequest;
    }
}


bool PrototypeAlgorithm::assignNodesAndLinks(Request * r) {
    Elements pool = network->getNodes();

    Elements unassignedNodes = Operation::filter(r->elementsToAssign(), Criteria::isComputational);
    for (Elements::iterator i = unassignedNodes.begin(); i != unassignedNodes.end(); i++) {
        Element * elem = (*i);
        elem->value = virtualElementValue(elem);
    }

//    Elements serverLayered = Operation::filter(r->elementsToAssign(), Criteria::isServerLayered);
//    if ( !slAssignment(serverLayered, pool, r) ) {
//        fprintf(stdout, "----------------[ERROR] server layer requirement failed\n---------------------");
//        return false;
//    }



//    for (Elements::iterator i = pool.begin(); i != pool.end(); i++) {
//        Element * elem = *i;
//        elem->value = physicalElementValue(elem, network);
//    }

//    PrototypeAlgorithm::linkChange = false;


    //ASSIGN_NODES
    if( !assignNodes(unassignedNodes, pool, r))
        return false;

    //ASSIGN_LINKS
    bool result = assignLinks(r);
//    if (result)
//        PrototypeAlgorithm::linkChange = true;
//    else
//        PrototypeAlgorithm::linkChange = false;
    return result;
}


//=================================FIRST_STAGE=================================
bool PrototypeAlgorithm::assignNodes(Elements & virtualNodes, Elements & pool, Request * r) {
    Elements nodes = Operation::filter(virtualNodes, Criteria::isUnassigned);
    while ( !nodes.empty() ) {
        Element * unassignedSeed = getSeedElement(nodes);
        std::deque<Element *> queue;
        queue.push_back(unassignedSeed);
        while ( !queue.empty() ) {
            Element * nextToAssign = queue.front();
            queue.pop_front();

            if ( nextToAssign->isAssigned() )
                continue;

            if ( nextToAssign->isSwitch() && !nextToAssign->isRouter())
                continue;

            if ( !assignSeedElement(nextToAssign, pool) ) {
                std::cout << "result == FALSE THEN EXHAUSTIVESEARCHER" << std::endl;
                nextToAssign->unassign();
                Elements assignedElements = Operation::filter(r->assignedElements(), Criteria::isComputational);
                if ( !exhaustiveSearch(nextToAssign, pool, assignedElements, true) ) {
                    return false;
                }
            }

            Elements adjacentNodes = Operation::filter(nextToAssign->adjacentNodes(), Criteria::isUnassigned);
            Elements adjacentEdges = nextToAssign->adjacentEdges();
            std::vector<Element * > tmp;
            tmp.insert(tmp.end(), adjacentEdges.begin(), adjacentEdges.end());
            std::sort(tmp.begin(), tmp.end(), Criteria::elementWeightDescending);

            for(size_t e = 0; e < tmp.size(); e ++) {
                Edge * edge = tmp[e]->toEdge();
                for (Elements::iterator i = adjacentNodes.begin(); i != adjacentNodes.end(); i++){
                    if (edge->connects(*i)) {
                        queue.push_back(*i);
                        adjacentNodes.erase(*i);
                        break;
                    }
                }
            }

//            for ( Elements::iterator i = adjacentNodes.begin(); i != adjacentNodes.end(); i++) {
//                queue.push_back(*i);
//            }
            nodes.erase(nextToAssign);
        }
    }

    return true;
}


//=================SECOND_STAGE========================

bool PrototypeAlgorithm::assignLinks(Request * request) {
    Elements edges = request->getTunnels();
    //std::cout << " --------------------" << edges.size() << "---------------------" << std::endl;

    std::vector<Element *> tunnels;
    tunnels.insert(tunnels.end(), edges.begin(), edges.end());
    std::sort(tunnels.begin(), tunnels.end(), Criteria::elementWeightDescending);

    //std::cout << " --------------------" << tunnels.size() << "---------------------" << std::endl;

    for(size_t e = 0; e < tunnels.size(); e ++) {
        Element * edge = tunnels[e];
        Link * tunnel = edge->toLink();
        if (tunnel->isAssigned())
            continue;

        Element * start = tunnel->getFirst()->getParentNode()->getAssignee();
        Element * end = tunnel->getSecond()->getParentNode()->getAssignee();

        BSearcher searcher(start, end, tunnel);
        if ( start == end ) {
            Path emptyPath = Path(end, start);
            tunnel->setRoute(emptyPath);
            tunnel->setAssignedFlag(true);
            continue;
        }

        if ( !searcher.isValid() ) {
            continue;
        }

        if ( !searcher.search() ) {
            return false;
        }

        Path route = searcher.getPath();
        tunnel->setRoute(route);
        tunnel->setAssignedFlag(true);
    }

    std::cout << "++++++++++++++++++++++" << std::endl;
    return true;

}