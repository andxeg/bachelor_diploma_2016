#pragma once

#include "defs.h"
#include <list>

class Snapshot;
class Task;
class Network;

class Mixer {
public:
    Mixer();
    bool read(const Snapshot & from, const Snapshot & to);
    void print() const;
    std::list<Task *> getTasks() const { return tasks; }
    Network * getNetwork() const { return network; }
private:
    Network * network;
    std::list<Task *> tasks;     
};
