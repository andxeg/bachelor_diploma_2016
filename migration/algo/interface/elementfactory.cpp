#include "elementfactory.h"

#include "computer.h"
#include "store.h"
#include "switch.h"
#include "link.h"

#include "parameter.h"

#include <stdio.h>

#define EPS 0.000000001

QMap<QString, Parameter *> ElementFactory::parameters;
QMap<Parameter*, double> ElementFactory::maxValues;
QMap<Parameter*, double> ElementFactory::resourcesSum;
QMap<Parameter*, double> ElementFactory::resourcesDeficit;

void ElementFactory::debugPrint(Element * element) {
    if ( element->isPhysical() )
        return;

    printf("Element %p of type %d assigned to element %p\n", 
            element, element->type, element->assignee);
}

Element * ElementFactory::populate(Element * element, const QMap<QString, ParameterValue *> & pr, bool isVirtual)
{
    Parameters params = parametersFromProperties(pr, isVirtual);
    element->parameters = params;

    return element;
}

Parameters ElementFactory::parametersFromProperties(const QMap<QString, ParameterValue *> & pr, bool isVirtual) {
    Parameters result;
    foreach(QString name, pr.keys()) {
        Parameter * par = parameterByName(name);
        ParameterValue * val = pr[name];
        result[par] = val;

        if ( isVirtual ) {
            double weight = val->weight(),
                    physical = resourcesSum.contains(par) ? resourcesSum.value(par) : 0.0;
            // counting deficit
            if ( physical > EPS ) {
                if ( resourcesDeficit.contains(par) )
                    resourcesDeficit[par] += weight / physical;
                else
                    resourcesDeficit[par] = weight / physical;
            }

            // counting max virtual value
            if ( !maxValues.contains(par) || maxValues[par] < weight )
                maxValues[par] = weight;
        } else {
            if ( !resourcesSum.contains(par) )
                resourcesSum[par] = val->weight();
            else
                resourcesSum[par] += val->weight();
        }
    }
    
    return result;
}

Parameter * ElementFactory::parameterByName(const std::string & name) {
    return parameterByName(QString::fromStdString(name));
}

Parameter * ElementFactory::parameterByName(const QString & name) {
    if ( parameters.contains(name) )
        return parameters[name];

    Parameter * parameter = new Parameter(name.toStdString());
    parameters.insert(name, parameter);
    return parameter;
}
