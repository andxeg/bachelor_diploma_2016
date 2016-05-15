#include "bfsqueue.h"

#include "element.h"
#include "edge.h"

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
    if ( isExhausted() ) return 0;

    Element * next = unvisited.front();
    unvisited.pop();

    if ( next == start || next->isNetwork() ) {
       Elements adjacentNodes = next->adjacentNodes();
       for( Elements::iterator i = adjacentNodes.begin(); i != adjacentNodes.end(); i++) {
          Element * node = *i;
          if ( ancestors.count(node) != 0 )
             continue;

          unvisited.push(node);
          ancestors[node] = next;
       }
    }

    return next;
}

Element * BFSQueue::getNextCandidate() {
    while ( !isExhausted() ) {
        return processNextItem();
    }

    return 0;
}

Path BFSQueue::getPath(Element * target) const {
    if ( ancestors.count(target) == 0 )
        return Path();

    Path result(target, start);
    Element * next = target;
    while ( next != start ) {
        Element * ancestor = ancestors.at(next);
        Elements ancestorEdges = ancestor->adjacentEdges();
        Elements::iterator i;
        for ( i = ancestorEdges.begin(); i != ancestorEdges.end(); i++) {
            Edge * edge = (*i)->toEdge();
            if ( edge->connects(next) ) {
                result.addElement(edge);
                break;
            }
        }
        if ( ancestor != start )
            result.addElement(ancestor);
        next = ancestor;
    }

    result.revert();
    return result;
}
