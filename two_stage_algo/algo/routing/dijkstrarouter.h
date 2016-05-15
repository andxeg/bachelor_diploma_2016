#pragma once
#include <map>
#include <vector>
#include <set>
#include <tuple>
#include <algorithm>
#include "defs.h"

class DijkstraRouter {
public:
    DijkstraRouter( Element * start, Network * n);
    void route();
    std::map<Element *, std::vector <Element * > > getPathes();
private:
    Element * start;
    Network * network;
    std::map<Element *, std::vector <Element * > > pathes;
};
