#pragma once

#include "defs.h"
#include "path.h"

#include <map>
#include <queue>

class BSearcher {
public:
    BSearcher(Element * start, Element * end, Element * tunnel = 0); 
    bool search();
    bool isValid() const;
    Path getPath() const;
    Element * getTunnel() const { return tunnel; }
private:
    Element * start;
    Element * end;
    Element * tunnel;
    std::queue<Element *> unvisited;
    std::map<Element *, Element *> ancestors;
};
