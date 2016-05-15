#pragma once

#include "defs.h"

#include "bfsqueue.h"

#include <map>

class BFSRouter {
public:
    typedef std::map<Element *, Elements *> Candidates; 
    typedef std::map<Element *, BFSQueue *> Searchers;
public:
    BFSRouter(Request & r, Element * target);
    ~BFSRouter();
    bool isExhausted() const; 
    bool isValid() const {
        return !searchers.empty(); 
    }
    bool search();

private:
    Searchers::iterator findNextNonExhausted(Searchers::iterator last) {
        Searchers::iterator i = last;
        while ( !isExhausted() ) {
            i++;

            if ( i == searchers.end() )
                i = searchers.begin();

            if ( i == last )
                return searchers.end();

            if ( !i->second->isExhausted() )
                return i;
        }
    }

    Elements intersectCandidates();
    bool commit(Elements & candidates);
    void discard(Elements & candidates);
private:
    Element * target;
    Candidates candidates;
    Searchers searchers;
};
