#pragma once

#include "defs.h"
#include <list>

class Task {
public:
    Task(Element * target, Element * destination); 
    bool isValid() const;
    bool isComplete() const;
    bool move(Element * newDestinaton);

    Element * getTarget() const;
    Element * getSource() const;
    Element * getDestination() const;
private:
    Element * target;
    Element * destination;
};
