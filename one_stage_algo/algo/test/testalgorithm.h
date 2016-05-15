#pragma once

#include <algorithm.h>

class TestAlgorithm : public Algorithm
{
public:
    TestAlgorithm(Network * network, const Requests & requests)
    :
       Algorithm(network, requests)
    {}
    virtual void schedule();
    void assignRequest(Request * r);
};
