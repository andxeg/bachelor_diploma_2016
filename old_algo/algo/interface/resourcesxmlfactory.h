#pragma once

#include "defs.h"
#include "factory.h"

class QDomElement;
class NetworkXMLFactory;

class ResourcesXMLFactory {
public:

    ResourcesXMLFactory(const QDomElement & element);
    virtual ~ResourcesXMLFactory();
    Network * getNetwork() const;

    class Element * getElement(const QString & name) const {
        if (ids.contains(name))
            return ids.value(name);
        return NULL;
    }
    class QString getName(Element*) const;
private:
    Network * network;
    QDomElement networkXml;
    Factory::ElementsMap elementsXML;
    Factory::IDS ids;
};
