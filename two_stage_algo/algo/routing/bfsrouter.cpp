#include "bfsrouter.h"

#include "operation.h"
#include "request.h"
#include "criteria.h"
#include "path.h"
#include "link.h"

#include <stdio.h>

BFSRouter::BFSRouter(Request & r, Element * t)
:
    target(t)    
{
    // if ( !Operation::isIn(target, r.getNodes())) throw;

    Elements adjacentTunnels = target->adjacentEdges();

    for(Elements::iterator i = adjacentTunnels.begin(); i != adjacentTunnels.end(); i++) {
        Element * tunnel = *i;
        Element * adjacent = tunnel->toEdge()->getAdjacent(target);
        if ( !adjacent->isAssigned() )
            continue;

        Element * assignee = adjacent->getAssignee();
        
        candidates[adjacent] = new Elements();
        searchers[adjacent] = new BFSQueue(assignee, tunnel);
    }
        
}

BFSRouter::~BFSRouter() {
    for(Candidates::iterator i = candidates.begin(); i != candidates.end(); i++)
        delete i->second;
    for(Searchers::iterator i = searchers.begin(); i != searchers.end(); i++)
        delete i->second;
}

bool BFSRouter::isExhausted() const {
    for(Searchers::const_iterator i = searchers.begin(); i != searchers.end(); i++) {
        BFSQueue * searcher = i->second;
        if ( !searcher->isExhausted() )
            return false;
    }

    return true;
}

bool BFSRouter::search() {
    Searchers::iterator i = searchers.begin();

    //std::cout << "START BFSRouter::search()" << std::endl;

    while( !isExhausted() ) {
        Element * start = i->first;
        BFSQueue * queue = i->second;
        Element * candidate = 0;

        while ( !queue->isExhausted()  ) {
            //std::cout << "BEFORE queue->getNextCandidate()" << queue << std::endl;
            candidate = queue->getNextCandidate();
            //std::cout << "AFTER queue->getNextCandidate()" << queue << std::endl;
            if ( candidate->canHostAssignment(target) )
                break;
        }

        if ( candidate != 0 && candidate->canHostAssignment(target) ) {
            Elements * c = candidates.at(start);
            c->insert(candidate);
            //std::cout << "BEFORE intersectCandidates()" << std::endl;
            Elements intersection = intersectCandidates();
            //std::cout << "AFTER intersectCandidates()" << std::endl;
            if ( !intersection.empty() )
                if ( commit(intersection) ) {
                    //std::cout << "END BFSRouter::search()" << std::endl;
                    return true;
                }
                else
                    discard(intersection);
        }

        i = findNextNonExhausted(i);
//        if (i == searchers.end())
//            std::cout << "i == searchers.end()" << std::endl;
    }

    //std::cout << "END BFSRouter::search()" << std::endl;
    return false;
}

Elements BFSRouter::intersectCandidates() {
    Elements result = *(candidates.begin()->second);
    for ( Candidates::iterator i = candidates.begin(); i != candidates.end(); i++) {
        Elements * c = i->second;
        result = Operation::intersect(result, *c);
    }
    return result;
}

bool BFSRouter::commit(Elements & c) {
    //std::cout << "START BFSRouter::commit(Elements & c)" << std::endl;
    bool flag;
    for(Elements::const_iterator j = c.begin(); j != c.end(); j++) {
        Element * host = *j;
        if ( !host->canHostAssignment(target) )
           continue;

        flag = true;

        for(Searchers::const_iterator i = searchers.begin(); i != searchers.end(); i++ ) {
            BFSQueue * queue = i->second;
            bool correct = true;
            Path route = queue->getPath(host, correct);
            if (!correct) {
                //delete links assignment
                for(Searchers::const_iterator k = searchers.begin(); k != searchers.end(); k++ ) {
                    Link * link = k->second->getTunnel()->toLink();
                    Operation::unassignLink(link);
                }
                //
                flag = false;
                break;//try another location from intersection
            } else {
                queue->getTunnel()->toLink()->setRoute(route);
                queue->getTunnel()->toLink()->setAssignedFlag(true);
            }

        }

        if (flag) {
            //Successfull assignment
            host->assign(target);
            //std::cout << "END BFSRouter::commit(Elements & c)" << std::endl;
            return true;
        }

    }

    //std::cout << "END BFSRouter::commit(Elements & c)" << std::endl;
    return false;
}

void BFSRouter::discard(Elements & d) {
//    std::cout << "START BFSRouter::discard" << std::endl;
    for ( Candidates::iterator i = candidates.begin(); i != candidates.end(); i ++) {
        Elements * c = i->second;
        Elements newElements = Operation::minus(*c, d);
        *c = newElements;
    }
//    std::cout << "END BFSRouter::discard" << std::endl;

}
