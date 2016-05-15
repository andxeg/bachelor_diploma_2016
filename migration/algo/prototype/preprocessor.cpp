#include "preprocessor.h"

#include "request.h"
#include "switch.h"
#include "link.h"
#include "operation.h"
#include "criteria.h"

#include "stdio.h"

//
#include "edge.h"
//

Request * Preprocessor::fakeNetElements(Request * r) {
    Request * fakeRequest = new Request(*r);
    Elements netElements = Operation::filter(r->getElements(), Criteria::isSwitch);
    for ( Elements::iterator i = netElements.begin(); i != netElements.end(); i++ ) {
        Switch * netElement = (*i)->toSwitch();
        if ( netElement->isRouter() )
            continue;


        Elements adjacentElements = netElement->adjacentNodes();
        while( !adjacentElements.empty() ) {
            Element * pivot = *(adjacentElements.begin());
            adjacentElements.erase(pivot); 
	    
	    /*
		//find link switch-pivot and switch-anchor
		//fake link will have thoughput = 
		min{ throughput switch-pivot, throughput switch-anchor }
		*/
	    //
	    //find link switch-pivot
            Element * link_switch_to_pivot = 0;
	    Elements switch_adjacent_edges = netElement->adjacentEdges();
	    for ( Elements::iterator e = switch_adjacent_edges.begin(); e != switch_adjacent_edges.end(); e++ ) {
		if ( (*e)->toEdge()->connects( netElement ) && (*e)->toEdge()->connects( pivot ) ) {
			link_switch_to_pivot = *e;
			break;
		}
	    }
	    //
	    
	    for (Elements::iterator a = adjacentElements.begin(); a != adjacentElements.end(); a++) {
                Element * anchor = *a;
                Link * fake = getFakeLink(pivot, anchor);
		
		//
		//find link switch-anchor
		Element * link_switch_to_anchor = 0;
		for ( Elements::iterator e = switch_adjacent_edges.begin(); e != switch_adjacent_edges.end(); e++ ) {
			if ( (*e)->toEdge()->connects( netElement ) && (*e)->toEdge()->connects( anchor ) ) {
				link_switch_to_anchor = *e;
				break;
			}
		}
		uint th1 = link_switch_to_pivot->toLink()->getThroughput();
		uint th2 = link_switch_to_anchor->toLink()->getThroughput();
		fake->setThroughput( th1 > th2 ? th2 : th1 );
		//
                
                fakeRequest->addExternalLink(fake);
            }
        } 

        Elements adjacentEdges = netElement->adjacentEdges();
        for (Elements::iterator e = adjacentEdges.begin(); e != adjacentEdges.end(); e++) {
            Link * link = (*e)->toLink();
            Port * port = link->getFirst();
            port->getParentNode()->toNode()->removePort(port);
            port = link->getSecond();
            port->getParentNode()->toNode()->removePort(port); 
            fakeRequest->omitElement(link);
        } 
        fakeRequest->omitElement(netElement);
    }
    printf("Faking request: %d elements against %d in the original\n", fakeRequest->size(), r->size());
    return fakeRequest;
}

Link * Preprocessor::getFakeLink(Element * first, Element * second) {
    Link * link = new Link();
    Port * port1 = new Port("dummy", first);
    Port * port2 = new Port("dummy", second);
    first->toNode()->addPort(port1);
    second->toNode()->addPort(port2);

    link->connect(port1, port2);
    port1->connect(link, port2);
    port2->connect(link, port1);
    return link;
}
