#pragma once

#include "defs.h"
#include "node.h"
#include "criteria.h"

#include <vector>
#include <algorithm>

class Path {
public:
    Path() : from(0), to(0) {}
    Path(Element * begin, Element * end) {
        if ( !Criteria::isPhysical(begin) ) throw;
        if ( !Criteria::isNode(begin) ) throw;
        if ( !Criteria::isPhysical(end) ) throw;
        if ( !Criteria::isNode(end) ) throw;

        from = begin;
        to = end;
    }

    inline bool isZeroPath() const {
        return from == to;
    }

    inline int length() const {
        path.size() + 2; 
    }

    inline bool isValid() const {
        return from != 0 && to != 0;
    }

    inline bool addElement(Element * element) {
        if ( !Criteria::isPhysical(element) ) return false;
        /*
        if ( path.empty() ) {
            if ( !Criteria::isLink(element) ) return false;
            Link * link = element->toLink();
            if ( !link->connects(from) ) return false;
        } else {
            if ( Criteria::isLink(element) ) {
                Link * link = element->toLink();
                if ( !link->connects(path.back()) ) return false; 
            } else if ( Criteria::isSwitch(element) ) {
                Switch * sw = element->toSwitch();
                if ( !sw->hasEdge(path.back()) ) return false;
            } else {
                return false;
            }
        }
        */
        path.push_back(element);
        return true;
    }

    inline void revert() {
        if ( !isValid() )
            return;

        Element * tmp = from;
        from = to;
        to = tmp;//previously was -> to = from;
        std::reverse(path.begin(), path.end()); 
    }

    std::vector<Element *> getPath() const { 
        return path;
    }

private:
    std::vector<Element *> path;
    Element * from;
    Element * to;
};
