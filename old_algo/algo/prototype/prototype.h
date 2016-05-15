#pragma once

#include "algorithm.h"

//
#include <string>
//


#include <vector>
#include <deque>
#include "exhaustivesearcher.h"

class PrototypeAlgorithm : public Algorithm {
    friend class ExhaustiveSearcher;
    friend class Snapshot;
public:
    PrototypeAlgorithm(Network * n, const Requests & r)
    : Algorithm(n, r) {}


    virtual void schedule();
    void oneStageSchedule();
    void twoStageSchedule();
    void oldAlgorithm();
    
    //
    void setResources(const Resources & r) { resources = r; }
    void setTenants(const TenantsElements & t) { tenantsElements = t; }

    //

private:
    std::vector<Request *> prioritizeRequests(Requests & r);
    bool scheduleRequest(Request * r);
    bool exhaustiveSearch(Element * e, Elements & pool, Elements & assignedElements, const bool & twoStage = false);
    bool assignSeedElement(Element * e, Elements & pool);
    Elements connectedComponent(Element * e);
    void tweakQueue(std::deque<Element *> & queue, Request * r);

    /* Divergent assignment procedures */
    bool routedAssignment(Elements & nodes, Elements & pool, Request * r);
    static bool simpleIncreasing(Request * first, Request * second);
    static bool requestCompare(Request * first, Request * second);
    static bool descendingVirt(const Element * first, const Element * second);
    static bool ascendingPhys(const Element * first, const Element * second);
    static double physicalElementValue(Element * element, Network * network);
    static bool descendingPhys(const Element * first, const Element * second);

    Element * getSeedElement(Elements & e, bool isVirtual = true);

    bool oldAssignNodesLinks(Elements & virtualNodes, Elements & pool, Request * r);


    bool createLinks(Element * assignedElement);
    bool newCreateLinks(Element * assignedElement);

    bool assignNodesAndLinks(Request * r);
    bool assignNodes(Elements & virtualNodes, Elements & pool, Request * r);
    static double virtualElementValue(Element * element);


    bool assignLinks(Request * request);

//
private:
	Resources resources;
	TenantsElements tenantsElements;
//    static bool linkChange;
//
};
