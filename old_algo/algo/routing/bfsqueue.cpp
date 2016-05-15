#include "bfsqueue.h"

#include "element.h"
#include "edge.h"
#include "link.h"
#include <stdio.h>
#include <iostream>
#include "operation.h"

BFSQueue::BFSQueue(Element * s, Element * t) 
: 
    start(s)
{
    if ( !t->isLink() ) throw;
    tunnel = t;

    unvisited.push(s);
    ancestors[s] = 0;
}

Element * BFSQueue::processNextItem() {
    //std::cout << "START BFSQueue::processNextItem()" << std::endl;
    if ( isExhausted() ) {
        //std::cout << "END BFSQueue::processNextItem()" << std::endl;
        return 0;
    }

    //std::cout << "is not exhausted()" << std::endl;
    //std::cout << unvisited.size() << std::endl;

    Element * next = unvisited.front();
    unvisited.pop();

    //std::cout << unvisited.size() << std::endl;


    //std::cout << "NEXT-> " << next << std::endl;
    if ( next == start || next->isNetwork() ) {
        //if next == link then check throughput
        //through this link path is not exist
        //this link returned but in BFSRouter::search() canHostAssignment will return false,
        // because next has type Link and we search storage or server
        if (next->isLink()) {
            if (!next->canHostAssignment(tunnel)) {
                //std::cout << "END BFSQueue::processNextItem()" << std::endl;
                return next;
            }
        }
        //

        if (next->isNode()) {
//            Elements adjacentNodes = next->adjacentNodes();
//            Elements adjacentEdges = next->adjacentEdges();
//            for (Elements::iterator i = adjacentNodes.begin(); i != adjacentNodes.end(); i++) {
//                Element * node = *i;
//                if (ancestors.count(node) != 0)
//                    continue;
//
//                //if link between node and next less than tunnel throughput then don't push in queue
//                bool flag = false;
//                for (Elements::iterator j = adjacentEdges.begin(); j != adjacentEdges.end(); j++) {
//                    Edge * edge = (*j)->toEdge();
//                    if (edge->connects(node)) {
//                        if (edge->toLink()->canHostAssignment(tunnel))
//                            flag = true;
//                        break;
//                    }
//                }
//
//                if (flag) {
//                    unvisited.push(node);
//                    ancestors[node] = next;
//                } else {
////                    std::cout << "FLAG == FALSE" << std::endl;
//                }
//            }

            Elements adjacentEdges = next->adjacentEdges();
            Elements edges;
            for (Elements::iterator j = adjacentEdges.begin(); j != adjacentEdges.end(); j++) {
                Edge * edge = (*j)->toEdge();
                if (edge->toLink()->canHostAssignment(tunnel))
                    edges.insert(edge);
            }

            for (Elements::iterator j = edges.begin(); j != edges.end(); j++) {
                Element * node = (*j)->toEdge()->getAdjacent(next);

                if (ancestors.count(node) != 0)
                    continue;

                unvisited.push(node);
                ancestors[node] = next;
            }

        } else if (next->isLink()) {
            if (!next->canHostAssignment(tunnel)) {
                //std::cout << "END BFSQueue::processNextItem()" << std::endl;
                return next;
            }
            Elements adjacentNodes = next->adjacentNodes();
            for (Elements::iterator i = adjacentNodes.begin(); i != adjacentNodes.end(); i++) {
                Element *node = *i;
                if (ancestors.count(node) != 0)
                    continue;
                unvisited.push(node);
                ancestors[node] = next;
            }
        }

    }

    //std::cout << "END BFSQueue::processNextItem()" << std::endl;

    return next;
}

Element * BFSQueue::getNextCandidate() {
    while ( !isExhausted() ) {
        return processNextItem();
    }

    return 0;
}

Path BFSQueue::getPath(Element * target, bool & correct) const {
    if ( ancestors.count(target) == 0 ) {
        correct = true;
        return Path();
    }

    Path result(target, start);
    Element * next = target;
    while ( next != start ) {
        Element * ancestor = ancestors.at(next);
        Elements ancestorEdges = ancestor->adjacentEdges();
        Elements::iterator i;
        for ( i = ancestorEdges.begin(); i != ancestorEdges.end(); i++) {
            Edge * edge = (*i)->toEdge();
            if ( edge->connects(next) ) {
                //We must assign tunnel on this physical edge
                if (!edge->canHostAssignment(tunnel)) {
                    //printf("[ERROR] In BFSQueue tunnel cannot assigned on found path.\n");
                    //delete assignment from current path
                    tunnel->toLink()->setRoute(result);
                    Operation::unassignLink(tunnel);
                    correct = false;
                    return Path();
                }

                edge->toLink()->assignLink(tunnel);
                //std::cout << "IN BFSQueue::getPath after link assignment" << std::endl;
                //

                result.addElement(edge);
                break;
            }
        }
        if ( ancestor != start )
            result.addElement(ancestor);
        next = ancestor;
    }

    correct = true;
    result.revert();
    return result;
}
