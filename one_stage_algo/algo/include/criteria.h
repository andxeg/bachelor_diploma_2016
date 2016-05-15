#pragma once

#include "element.h"

class Criteria {
public:
    inline static bool isComputer(const Element * e) { 
        return e->isComputer();
    }

    inline static bool isStore(const Element * e) {
        return e->isStore();
    }

    inline static bool isSwitch(const Element * e) { 
        return e->isSwitch();
    }

    inline static bool isLink(const Element * e) { 
        return e->isLink();
    }

    inline static bool isComputational(const Element * e) {
        return e->isComputational();
    }

    inline static bool isNetwork(const Element * e) {
        return e->isNetwork();
    }

    inline static bool isNode(const Element * e) {
        return e->isNode();
    }

    inline static bool isEdge(const Element * e) {
        return e->isEdge();
    }

    inline static bool isPhysical(const Element * e) {
        return e->isPhysical();
    }

    inline static bool isVirtual(const Element * e) {
        return e->isVirtual();
    }

    inline static bool isAvailable(const Element * e) {
        return e->isAvailable();
    }

    inline static bool isAssigned(const Element * e) {
        return e->isAssigned();
    }

    inline static bool isUnassigned(const Element * e) {
        return !isAssigned(e);
    }

    inline static bool isAdjacent(const Element * e, const Element * t) {
        return e->isAdjacent(t); 
    }

    inline static bool elementWeightDescending(const Element * first, const Element * second) {
        return first->weight() > second->weight(); 
    }

    inline static bool elementWeightAscending(const Element * first, const Element * second) {
        return first->weight() < second->weight();
    }

    inline static bool canHostAssignment(const Element * host, const Element * target) {
        return host->canHostAssignment(target);
    }

    inline static bool isExhaustiveCandidate(const Element * host, const Element * target) {
        if ( !host->typeCheck(target) )
            return false;
        return host->attributeCheck(target);
    }

    static bool isServerLayered(const Element * e);
    static bool isDCLayered(const Element * e);
};
