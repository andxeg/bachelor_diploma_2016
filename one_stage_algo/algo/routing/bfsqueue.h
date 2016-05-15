#pragma once

#include "defs.h"
#include "path.h"

#include <map>
#include <queue>

class BFSQueue {
public:
    BFSQueue(Element * start, Element * tunnel);
    inline bool isExhausted() const { return unvisited.empty(); }
    Element * getNextCandidate();
    Path getPath(Element * target, bool & correct) const;
    Element * getTunnel() const { return tunnel; }
    unsigned long getUnvisitedSize() { return unvisited.size(); }
private:
    Element * processNextItem();
private:
    Element * start;
    Element * tunnel;
    std::queue<Element *> unvisited;
    std::map<Element *, Element *> ancestors;
};
