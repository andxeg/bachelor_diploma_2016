#pragma once

#include <defs.h>

#include <QMap>
#include <QString>
#include <QVariant>

#include <string>

class ElementFactory
{
public:
    static void debugPrint(Element * element);
    static Element * populate(Element * element, const QMap<QString, ParameterValue *> &, bool isVirtual = false);
    static Parameters parametersFromProperties(const QMap<QString, ParameterValue *> &, bool isVirtual = false);
    static Parameter * parameterByName(const std::string & name);
    static Parameter * parameterByName(const QString & name);

    static inline double getDeficit(Parameter* param) {
        return resourcesDeficit.value(param);
    }

    static inline double getMaxValue(Parameter* param) {
        return maxValues.value(param);
    }

private:
    static QMap<QString, Parameter *> parameters;

    // maximum values of parameter (converted to double); used for normalization
    static QMap<Parameter*, double> maxValues;

    // sum of physical resources; used to compute deficit
    static QMap<Parameter*, double> resourcesSum;

    // deficit of resources,
    // calculated as sum of all required resources divided by all physical resources
    static QMap<Parameter*, double> resourcesDeficit;
};
