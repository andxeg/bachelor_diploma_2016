#include "export.h"
#include "elementfactory.h"

double EF::getDeficit(Parameter * p)
{
    return ElementFactory::getDeficit(p);
}

double EF::getMaxValue(Parameter * p)
{
    return ElementFactory::getMaxValue(p);
}
