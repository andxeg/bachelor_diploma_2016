#pragma once

#include "edge.h"
#include "criteria.h"
#include "path.h"

//
#include "element.h"
//

class Link : public Edge {
    friend class ElementFactory;
    friend class Switch;
public:
    enum Attributes {
        NONE = 0
    };

    enum Latencies {
        DUMMY = 0,
        NORMAL = 1,
        AFFINITY = NORMAL << 1
    };

    Link() : Edge(), throughput(0), latency(DUMMY) {
        type = LINK;
    }

    void setThroughput(const unsigned & throughput) {
        this->throughput = throughput;
    }

    virtual bool setRoute(Path& route) {
        this->route = route;
        return true;
    }
    
    //
    uint getThroughput() {
	return throughput;
    }
    
    uint getFullThroughput() {
	    uint result=0;
	    for(Elements::iterator i=assignments.begin(); i!=assignments.end(); i++) {
		    result+=(*i)->toLink()->getThroughput();
	    }
	    return ( result + throughput );
    }
    
    uint linkCost() {//calculate busy throughput
	    return (getFullThroughput() - throughput);
    }
    
    bool assignLink(Element * other) {
	if ( !canHostAssignment(other) )
		return false;


        decreaseResources(other);
        
        assignments.insert(other);
        return true;
    }
    
    void unassignLink(Element * other) {
	Elements::iterator a = assignments.find(other);
	if ( a == assignments.end() )
		return;

	restoreResources(other);
	
	assignments.erase(a);
    }
    //
	
    virtual Path getRoute() const {
        return route;
    }

    virtual bool isAssigned() const {
        return isVirtual() && route.isValid();
    }

    virtual void unassign() {
        
	 if ( !isAssigned() )
           return;
	
        route = Path();
        
	
    }

    virtual Latencies getLatency() const {
        return latency;
    }

    virtual void setLatency(Latencies l) {
        latency = l;
    }

    virtual bool isDummy() const {
        return latency == DUMMY;
    }

    virtual bool isAffine() const {
        return latency == AFFINITY;
    }

private:
    virtual bool typeCheck(const Element * other) const {
        return Criteria::isNetwork(other);
    }

    virtual bool physicalCheck(const Element * other) const {
        Link * link = other->toLink();
        if ( throughput < link->throughput ) return false;
        return true;
    }

    virtual void decreaseResources(const Element * other) {
        Link * link = other->toLink();
        throughput -= link->throughput;
    }

    virtual void restoreResources(const Element * other) {
        Link * link = other->toLink();
        throughput += link->throughput;
    }

private:
    unsigned throughput;
    Latencies latency; // TODO: do not know how to check this yet, this might be a constant value maybe
    Path route;
};
