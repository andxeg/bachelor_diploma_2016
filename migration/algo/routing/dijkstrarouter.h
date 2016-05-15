#pragma once

#include "router.h"

class DijkstraRouter : public Router {
public:
    DijkstraRouter(Link * tunnel, Element * from, Element * to, Network * n);
    virtual bool route();
private:
    Element * from;
    Element * to;
    Link * tunnel;
};
