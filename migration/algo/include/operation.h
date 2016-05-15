#pragma once

#include "defs.h"
#include "element.h"

namespace Operation {

    inline Elements filter(const Elements & elements, bool (*criterion)(const Element *)) {
        Elements result;
        for ( Elements::iterator e = elements.begin(); e != elements.end(); e++) {
            Element * element = *e;
            if (criterion(element)) {
                result.insert(element);
            } 
        }
        return result;
    } 

    inline Elements filter(const Elements & elements, const Element * target, bool (*criterion)(const Element *, const Element *)) {
        Elements result;
        for ( Elements::iterator e = elements.begin(); e != elements.end(); e++) {
            Element * element = *e;
            if ( criterion(element, target) )
                result.insert(element);
        }
        return result;
    }

    inline Elements intersect(const Elements & first, const Elements & second) {
        Elements result;
        if ( first.empty() ) return result;
        if ( second.empty() ) return result;
        const Elements & less = first.size() < second.size() ? first : second;
        const Elements & more = first.size() < second.size() ? second : first;
        for ( Elements::const_iterator e = less.begin(); e != less.end(); e++ )
            if ( more.find(*e) != more.end() )
                result.insert(*e);
        return result;
    }

    inline Elements join(const Elements & first, const Elements & second) {
        if ( first.empty() ) return second;
        if ( second.empty() ) return first;
        Elements result(first);
        result.insert(second.begin(), second.end());
        return result;
    }

    inline Elements minus(const Elements & first, const Elements & second) {
        if ( first.empty() ) return first;
        if ( second.empty() ) return first;
        Elements result(first);
        for ( Elements::iterator e = second.begin(); e != second.end(); e++ )
            result.erase(*e);
        return result;
    }

    inline bool isIn(const Element * element, const Elements & elements) {
        Element * cmp = const_cast<Element *>(element);
        return elements.find(cmp) != elements.end(); 
    }

    inline Elements unite(const Elements & first, const Elements & second) {
        Elements result(first);
        result.insert(second.begin(), second.end());
        return result;
    }

    inline void forEach(const Elements & elements, void (*op)(Element *)) {
        for ( Elements::iterator i = elements.begin(); i != elements.end(); i++ ) {
            Element * e = const_cast<Element *>(*i);
            op(e);
        }
    }

    inline void unassign(Element * e) {
        e->unassign();
    }
};
