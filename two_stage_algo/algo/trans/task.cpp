#include "task.h"
#include "element.h"

Task::Task(Element * t, Element * d)
:
    target(t),
    destination(d)
{
}

bool Task::isValid() const {
    if ( target == 0 || destination == 0 ) return false;
    if ( target->isPhysical() || destination->isVirtual() ) return false;
    if ( !target->isAssigned()) return false;
    return true;
}

bool Task::isComplete() const {
    if ( !isValid() ) return false;
    const Element * source = target->getAssignee();
    if ( source != destination ) return false;
    return true;
}

Element * Task::getTarget() const
{
    return target;
}

Element * Task::getSource() const
{
    return target->getAssignee();
}

Element * Task::getDestination() const
{
    return destination;
}
