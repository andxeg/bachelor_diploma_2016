#pragma once

#include <defs.h>

#include <string.h>

class Algorithm
{
protected:
    Algorithm(Network * n, const Requests & r)
    :
        network(n),
        requests(r)
    {
    
    }
public:
    virtual void schedule() = 0; 
protected:
    Network * network;
    Requests requests;
};
