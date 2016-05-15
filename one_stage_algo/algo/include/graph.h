#pragma once

#include "defs.h"
#include "operation.h"
#include "criteria.h"

class Graph {
public:
    inline const Elements & getElements() const {
        return elements;
    }

    inline Elements getNodes() const {
        return Operation::filter(getElements(), Criteria::isNode);
    }
    
    inline Elements getEdges() const {
        return Operation::filter(getElements(), Criteria::isEdge);
    }

    inline int size() const {
        return elements.size();
    }

protected:
    Elements elements;
};
