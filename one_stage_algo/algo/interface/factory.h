#pragma once

#include "defs.h"

#include <QMap>
#include <QVariant>
#include <QtXml/QDomElement>
#include <QStringList>

/*
 * Factory for creating resources both from tenant and from resources description.
 * XML representation is used.
 * These is a static class used for parsing xml files only.
 */
class Factory
{
public:
    typedef QMap<Element *, QDomElement> ElementsMap;
    typedef QMap<QString, Element *> IDS;
    typedef QMap<QString, QVariant> Properties;
    typedef QMap<QString, ParameterValue *> Params;
private:
    Factory() {}
    virtual ~Factory() {}

public:

    // These method is useful for any xml representation.
    // Returns: Map of all found elements on corresponding xml objects
    static ElementsMap getXmlElementsByTypes(const class QStringList &, const QDomElement &);
    static IDS getReverseIndex(const ElementsMap & index);

private:

    static void createElementsFromNodeList(QDomNodeList & list, ElementsMap& elementsMap);
    static Element * createElementFromXML(const QDomElement & element, const ElementsMap& elementsMap);
    static Properties getAttributesFromXML(const QDomNamedNodeMap &);
    static Params getParametersFromXML(const QDomNodeList &);

    static void addPortsFromXML(const QDomElement& element, Node* node);

    // Create link or node depending on type name
    static Link * createLink(const QDomElement & element, const ElementsMap& elementsMap);
    static Element * createNode(const QDomElement & element);
    static void setServerLayer(class LeafNode * node, const QDomElement & element);
    static void setDCLayer(class LeafNode * node, const QDomElement & element);
    static void setStorageClass(class Store * store, const QDomElement & element);

    static void setSwitchAttributes(Switch* sw, const QDomElement & e);
};
