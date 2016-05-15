#pragma once

#include "defs.h"
#include "path.h"

class Network;

class Router {
public:
    Router(Network * n) : network(n) {}
    virtual bool route() = 0;
protected:
    Network * network;
};
