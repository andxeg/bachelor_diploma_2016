#include "exhaustivesearcher.h"

#include "element.h"
#include "network.h"
#include "criteria.h"
#include "operation.h"
#include "path.h"
#include "link.h"
#include "port.h"
#include "routing/bsearcher.h"

#include <algorithm>
using std::vector;

#include <stdio.h>

//
#include "leafnode.h"
#include "migration.h"
#include "transmission.h"
//

ExhaustiveSearcher::ExhaustiveSearcher(Network * n, const Resources & res, const TenantsElements & tens, Elements & pool, Element * t, int d, int ma) 
:
    //
    network(n),
    resources(res),
    tenantsElements(tens),
    //
    target(t),
    maxAttempts(ma),
    attempt(0),
    depth(d)
{
    Elements cand = Operation::filter(pool, target, Criteria::isExhaustiveCandidate);
    if ( cand.size() < depth )
        depth = cand.size();

    candidates = vector<Element *>(cand.begin(), cand.end());
    indices = new int[depth + 1];
    for ( int i = 0; i < depth; i++ )
        indices[i] = i;
    indices[depth] = candidates.size();
    printf("Created exhaustive search environment to assign element %p, %d possible candidates\n", t, candidates.size());
}

ExhaustiveSearcher::~ExhaustiveSearcher() {
    printf("Exhaustive search decomposing, attempted %d of %d attempts,", attempt, maxAttempts);
    if ( isExhausted() )
        printf(" is exhausted\n");
    else
        printf(" not exhausted\n");

    delete indices;
}

bool ExhaustiveSearcher::isValid() const {
    if ( !target ) return false;
    if ( !target->isVirtual() ) return false;
    if ( !target->isNode() ) return false;
    return true;
}

bool ExhaustiveSearcher::isExhausted() const {
    return indices[depth - 1] == indices[depth];
}

bool ExhaustiveSearcher::search() {
    if ( !isValid() )
       return false;

    while( !isExhausted() ) {
        attempt++;
        if ( attempt > maxAttempts )
            return false;
        if ( makeAttempt() )
            return true;
    }

    return false;
}

bool ExhaustiveSearcher::makeAttempt() {
    if ( isExhausted() )
        return false;

    Elements cortege = getNextCortege();
    Assignments cache = getAssignmentsCache(cortege);
    Elements assignmentPack = getAssignmentPack(cache);
    Operation::forEach(assignmentPack, Operation::unassign);
    
    assignmentPack.insert(target);
    if ( performGreedyAssignment(assignmentPack, cortege) ) {
	
	if ( updatePathes(assignmentPack) ) {
        /*NOW WORK:
          * Migration plan
          * Task for Transmission -> {Ti} = < e, s, d >
          * Transmission -> {TRi} = < T, ts, tp, {Li}, tunnel >
          * {Li} is a set of physical links for migration
          * tunnel is a virtual channel for migration
          */
	      
	    //Перед миграцией target пока никуда не назначена
        assignmentPack.erase(target);
        Element * targetAssignee = target->getAssignee();
        target->unassign();
        //

	    Assignments newAssignment = getNewAssignment(assignmentPack);
	    Transmissions transmissions = getTransmissions(cache, newAssignment);

        //27 aug 2015 14:08
        //After performGreedyAssignment all virtual elements are situated on the destination physical elements
        //But before migration they must be on the source physical elements
        Operation::forEach(assignmentPack, Operation::unassign);
        for ( Assignments::iterator i = cache.begin(); i != cache.end(); i++) {
            if ( i->second->assign(i->first) == false )
                printf("Error in migration preparation\n");
        }
        //

	    Migration migration(network, transmissions, resources, tenantsElements);
	    if ( !migration.isValid()) {
            printf("There are not any transmissions with source != destination\n");
            return true;
	    }
	    
	    migration.print();
	    
	    if ( migration.createMigrationPlan() ) {
            //Миграция завершилась корректно, следовательно, можно назначать target
            if ( targetAssignee->assign(target) == false )
                printf("Error while assign target after creation migration plan");
            migration.print();
            return true;
	    }
	    
	    //Если миграция невозможна, следовательно, откатываемся к предыдущему назначению
	    printf("Migration is impossible\n");
            //
	}
	//если обновить пути не удалось, следовательно, отказываемся от назначения, которое было
	//предоставлено методом performGreedyAssignment
	assignmentPack.insert(target);//возвращаем рассматриваемый элемент, так как его назначение тоже нужно удалить
	
	Operation::forEach(assignmentPack, Operation::unassign);
	
    }
    
    for(Assignments::iterator i = cache.begin(); i != cache.end(); i++ ) 
        i->second->assign(i->first);

    return false;
}

Elements ExhaustiveSearcher::getNextCortege() {
    Elements result;
    for ( int i = 0; i < depth; i++ )
        result.insert(candidates[indices[i]]);

    advanceCursors();
    return result;
}

void ExhaustiveSearcher::advanceCursors() {
    int border = 0;
    for ( int i = depth - 1; i >= 0; i-- ) {
        if ( indices[i] != indices[i+1] - 1) {
            border = i;
            break;
        }
    }

    indices[border]++;
    for (int i = border + 1; i < depth; i++) {
        indices[i] = indices[i-1] + 1;
    }
}

ExhaustiveSearcher::Assignments ExhaustiveSearcher::getAssignmentsCache(Elements & resources) {
    Assignments result;
    for (Elements::iterator i = resources.begin(); i != resources.end(); i++) {
        Element * resource = *i;
        Elements assignments = resource->getAssignments();
        for(Elements::iterator a = assignments.begin(); a != assignments.end(); a++ ) {
            Element * assignment = *a;
	    //check the elements for which migration is not allowed
	    if ( assignment->isComputer() || assignment->isStore() ) {
		    LeafNode * node = (LeafNode *)assignment;
		    if ( node->getMigration() == 0 )
			    continue;
	    }
	    //
            result[assignment] = resource;
        }
    }
    return result;
}

Elements ExhaustiveSearcher::getAssignmentPack(ExhaustiveSearcher::Assignments & assignments) {
    Elements result;
    for (Assignments::iterator i = assignments.begin(); i != assignments.end(); i++) {
        result.insert(i->first);
    }
    return result;
}

bool ExhaustiveSearcher::performGreedyAssignment(Elements & t, Elements & p) {
    std::vector<Element *> targets(t.begin(), t.end());
    std::vector<Element *> physical(p.begin(), p.end());
    std::sort(targets.begin(), targets.end(), Criteria::elementWeightDescending);
    std::sort(physical.begin(), physical.end(), Criteria::elementWeightAscending);
    
//    printf("\n[PerformGreedyAssignment] START\n");
//    printf("Virtual elements\n");
//    for ( uint virtElem = 0; virtElem < targets.size(); virtElem ++ ) {
//	std::vector< std::string > assignInfo = tenantsElements[targets[virtElem]];
//	printf("\tTenant -> [%s]; Element -> [%s]\n", assignInfo[0].c_str(), assignInfo[1].c_str());
//    }
//
//    printf("Physical elements\n");
//    for ( uint physElem = 0; physElem < physical.size(); physElem ++ ) {
//	printf("\tPhysical element -> [%s]\n", resources[physical[physElem]].c_str());
//    }
//
//    printf("\n\n");
    
    for(vector<Element *>::iterator i = targets.begin(); i != targets.end(); i++) {
        Element * element = *i;
	
//	printf("Current element->[%s] from tenant [%s]\n", tenantsElements[element][1].c_str(), tenantsElements[element][0].c_str());
	
        bool result;
        for (vector<Element *>::iterator j = physical.begin(); j != physical.end(); j++) {
            Element * assignee = *j;
            result = assignee->assign(element);
            if ( result ) {
//		printf("Current element was assigned on [%s]\n", resources[assignee].c_str());
                break;
	        }
        }
        
        if ( !result ) {
            Operation::forEach(t, Operation::unassign);
//	    printf("Current element was not assigned\n");
//	    printf("\n[PerformGreedyAssignment] END\n");
            return false;
        }

        std::sort(physical.begin(), physical.end(), Criteria::elementWeightAscending);
	
	
//	printf("Physical elements after assignment\n");
//	for ( uint physElem = 0; physElem < physical.size(); physElem ++ ) {
//		printf("\tPhysical element -> [%s]\n", resources[physical[physElem]].c_str());
//	}
//
//	printf("\n\n");
	
    }

//    printf("\n[PerformGreedyAssignment] END\n");
    return true;;
}

bool ExhaustiveSearcher::updatePathes(Elements & assignments) {
    std::map<Link *, Path> oldPathes;
    for(Elements::iterator i = assignments.begin(); i != assignments.end(); i++ ) {
        Element * assignment = *i;
        Elements edges = assignment->adjacentEdges();
        for(Elements::iterator e = edges.begin(); e != edges.end(); e++ ) {
            Element * edge = *e;
            Link * tunnel = edge->toLink();
            Path oldPath = tunnel->getRoute();
            oldPathes[tunnel] = oldPath;

            Element * start = tunnel->getFirst()->getParentNode()->getAssignee();
            Element * end = tunnel->getSecond()->getParentNode()->getAssignee();
            BSearcher searcher(start, end, tunnel);
            if ( !searcher.isValid() ) 
                continue;
            if ( !searcher.search() ) {
                for(std::map<Link *, Path>::iterator p = oldPathes.begin();
                        p != oldPathes.end(); p++) 
                    p->first->setRoute(p->second);
                return false;
            }
            Path route = searcher.getPath();
            tunnel->setRoute(route);
        }
    }

    return true;
}

//
ExhaustiveSearcher::Assignments ExhaustiveSearcher::getNewAssignment(Elements & assignments) {
	Assignments result;
	for (Elements::iterator i = assignments.begin(); i != assignments.end(); i++) {
		Element * resource = (*i)->getAssignee();
		result[(*i)] = resource;
	}
	return result;
}

Transmissions ExhaustiveSearcher::getTransmissions(ExhaustiveSearcher::Assignments & oldAssignment, ExhaustiveSearcher::Assignments & newAssignment) {
	Transmissions result;
	for (Assignments::iterator i = oldAssignment.begin(); i != oldAssignment.end(); i++) {
		if ( i->second != newAssignment[i->first] ) {//if source != destination
			Transmission * transmission = new Transmission (i->first, i->second, newAssignment[i->first]);
			result.push_back( transmission );
		}
	}
	return result;
}








