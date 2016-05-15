#include "bsearcher.h"

#include "element.h"
#include "edge.h"
#include "link.h"
#include <iostream>

BSearcher::BSearcher(Element * s, Element * e, Element * t)
:
    start(s),
    end(e),
    tunnel(t)
{
}

bool BSearcher::isValid() const {
    if ( !start ) return false;
    if ( !start->isPhysical() ) return false;
    if ( !end ) return false;
    if ( !end->isPhysical() ) return false;
    if ( tunnel != 0 && !tunnel->isVirtual() && !tunnel->isLink() ) return false;
    return true;
}

bool BSearcher::search() {
    unvisited.push(start);
    ancestors[start] = 0;

    while ( !unvisited.empty() ) {
        Element * next = unvisited.front();
        unvisited.pop();

        if ( next == end ) {
//            std::cout <<  "BSearcher path found" << std::endl;
            return true;
        }

        if ( !next->isNetwork() && next != start )
           continue;

        if (next->isLink()) {
            if (!next->toLink()->canHostAssignment(tunnel)) {
                std::cout <<  "BSearcher tiny link" << std::endl;
                std::cout <<  "BSearcher tiny link" << std::endl;
                std::cout <<  "BSearcher tiny link" << std::endl;
                std::cout <<  "BSearcher tiny link" << std::endl;
                std::cout <<  "BSearcher tiny link" << std::endl;
                continue;
            }
        }

        if (next->isNode()) {
////            std::cout <<  "BSearcher NODE" << std::endl;
//            Elements adjacentNodes = next->adjacentNodes();
//            Elements adjacentEdges = Elements(next->adjacentEdges());
//            for (Elements::iterator i = adjacentNodes.begin(); i != adjacentNodes.end(); i++) {
//                Element * node = *i;
//                if (ancestors.count(node) != 0)
//                    continue;
//
//                //if link between node and next less than tunnel throughput then don't push in queue
//                bool flag = false;
//                Element * e = NULL;
//                for (Elements::iterator j = adjacentEdges.begin(); j != adjacentEdges.end(); j++) {
//                    Edge * edge = (*j)->toEdge();
//                    if (edge->connects(node)) {
//                        if (edge->toLink()->canHostAssignment(tunnel))
//                            flag = true;
//                        e = edge;
//                        break;
//                    }
//                }
//
//                adjacentEdges.erase(e);
//
//                if (flag) {
//                    unvisited.push(node);
//                    ancestors[node] = next;
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


        } else {
//            std::cout <<  "BSearcher LINK" << std::endl;
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

    return false;
}

Path BSearcher::getPath() const {
    if ( ancestors.count(end) == 0 )
        return Path();

    Path result(end, start);
    Element * next = end;
    while ( next != start ) {
        Element * ancestor = ancestors.at(next);
        Elements ancestorEdges = ancestor->adjacentEdges();
        Elements::iterator i;
        for ( i = ancestorEdges.begin(); i != ancestorEdges.end(); i++) {
            Edge * edge = (*i)->toEdge();
            if ( edge->connects(next) ) {
                //We must assign tunnel on this physical edge
                if (!edge->canHostAssignment(tunnel))
                    printf("[ERROR] In BSearcher tunnel cannot assigned on found path.\n");
                edge->toLink()->assignLink(tunnel);
                //

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
