#pragma once

#include "element.h"
#include "criteria.h"
#include "port.h"

class Edge : public Element {
protected:
    Edge() : Element(), first(0), second(0) {}
    virtual ~Edge() {
	}
public:
    Port * getFirst() const {
        return first;
    }

    Port * getSecond() const {
        return second;
    }

    bool connect(Port * first, Port * second) {
    	/*
        if ( Criteria::isPhysical(this) != Criteria::isPhysical(first)) return false;
        if ( Criteria::isPhysical(this) != Criteria::isPhysical(second)) return false;
        if ( !Criteria::isNode(first) ) return false;
        if ( !Criteria::isNode(second) ) return false; */

        this->first = first;
        first->connect(this, second);
        this->second = second;
        second->connect(this, first);
        return true;
    }

    bool connects(const Element * node) const {
        return getAdjacent(node) != 0;
    }

    Element * getAdjacent(const Element * node) const {
        if ( first->getParentNode() == node ) return second->getParentNode();
        if ( second->getParentNode() == node ) return first->getParentNode();
        return 0;
    }

    virtual Elements adjacent() const {
        Elements result;
        result.insert(first->getParentNode());
        result.insert(second->getParentNode());
        return result; 
    }

    virtual Elements adjacentNodes() const {
        return adjacent();
    }

    virtual Elements adjacentEdges() const {
        return Elements();
    }


protected:
    Port * first;
    Port * second;
};
