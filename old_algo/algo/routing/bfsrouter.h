#pragma once

#include "defs.h"

#include "bfsqueue.h"

#include <map>
#include <iostream>

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

            if ( i == last ) {

//                std::cout << "return searcher.end()" << std::endl;
//
//                size_t n = 0;
//                Searchers::iterator it;
//                for(Searchers::iterator j = searchers.begin(); j != searchers.end(); j++) {
//                    BFSQueue * searcher = j->second;
//                    if ( searcher->isExhausted() )
//                        n++;
//                    else
//                        it = j;
//                }

//                std::cout << "ALL searchers -> " << searchers.size() << " exhausted searchers -> " << n << std::endl;
//
//                if ( isExhausted())
//                    std::cout << "isExhausted()" << std::endl;
                //return searchers.end();
                return last;
            }

            if ( !i->second->isExhausted() ) {
                //std::cout << "return i" << std::endl;
                return i;
            }
        }

        //std::cout << "return nothing" << std::endl;
    }

    Elements intersectCandidates();
    bool commit(Elements & candidates);
    void discard(Elements & candidates);
private:
    Element * target;
    Candidates candidates;
    Searchers searchers;
};
