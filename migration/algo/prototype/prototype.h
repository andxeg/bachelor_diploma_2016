#pragma once

#include "algorithm.h"

//
#include <string>
//


#include <vector>
#include <deque>


class PrototypeAlgorithm : public Algorithm {
public:
    PrototypeAlgorithm(Network * n, const Requests & r)
    : Algorithm(n, r) {}

    virtual void schedule();
    
    //
    void setResources(const Resources & r) { resources = r; }
    void setTenants(const TenantsElements & t) { tenantsElements = t; }
    //
    
private:
    std::vector<Request *> prioritizeRequests(Requests & r);
    bool scheduleRequest(Request * r);
    bool exhaustiveSearch(Element * e, Elements & pool);
    bool assignSeedElement(Element * e, Elements & pool);
    Elements connectedComponent(Element * e);
    void tweakQueue(std::deque<Element *> & queue, Request * r);

    /* Divergent assignment procedures */
    bool routedAssignment(Elements & nodes, Elements & pool, Request * r);
    bool slAssignment(Elements & nodes, Elements & pool, Request * r);
    bool dlAssignment(Elements & nodes, Elements & pool, Request * r);
    bool dlAssignmentStrict(Elements & nodes, Elements & pool, Request * r);
    bool dlRequestAssignment(Request * r);
    bool slrAssignment(Elements & nodes, Elements & pool, Request * r);

    static bool simpleIncreasing(Request * first, Request * second);

    Element * getSeedElement(Elements & e, bool isVirtual = true);

//
private:
	Resources resources;
	TenantsElements tenantsElements;
//
};
