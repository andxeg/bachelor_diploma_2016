#include "factory.h"

#include <iostream>
#include "switch.h"
#include "store.h"
#include "computer.h"
#include "leafnode.h"
#include "port.h"
#include "parameter.h"

#include "elementfactory.h"

#include <QStringList>
#include <QDomNamedNodeMap>
#include <QDebug>

Factory::ElementsMap Factory::getXmlElementsByTypes(const QStringList & types, const QDomElement & root) {
    ElementsMap result;
    for (int i = 0; i < types.size(); i++ ) {
        QString type = types.at(i);
        QDomNodeList elementsList = root.elementsByTagName(type);
        createElementsFromNodeList(elementsList, result);
    }
    return result;
}

Factory::IDS Factory::getReverseIndex(const Factory::ElementsMap & index) {
    IDS result;
    foreach(Element * e, index.keys())
        result[index[e].attribute("name")] = e;
    return result;
}

void Factory::createElementsFromNodeList(QDomNodeList & list, ElementsMap& elementsMap) {
    for (int i = 0; i < list.size(); i++) {
        QDomElement xmlElement = list.at(i).toElement();
        Element* element = createElementFromXML(xmlElement, elementsMap);
        if ( element != 0 ) {
            elementsMap[element] = xmlElement;
        }
    }
}

Factory::Properties Factory::getAttributesFromXML(const QDomNamedNodeMap & m) {
    Properties result;
    for (int i = 0; i < m.length(); i++) {
        QDomNode node = m.item(i);
        result.insert(node.nodeName(), node.nodeValue());
    }
    return result;
}

Factory::Params Factory::getParametersFromXML(const QDomNodeList & l) {
    Params result;
    for (int i = 0; i < l.length(); i++) {
        QDomElement e = l.at(i).toElement();
        if ( e.tagName() != "parameter" )
            continue;

        QString parameterName = e.attribute("parameter_name");
        QString parameterType = e.attribute("parameter_type");
        QVariant parameterValue = e.attribute("parameter_value");
        ParameterValue * pv = 0;

        if ( parameterType == "integer" )
            pv = new ParameterInt(parameterValue.toInt());
        else if ( parameterType == "real" )
            pv = new ParameterReal(parameterValue.toFloat());
        else if ( parameterType == "string" ) {
            QString stringValue = parameterValue.toString();
            std::string conversedString = stringValue.toStdString();
            pv = new ParameterString(conversedString);
        }

        result.insert(parameterName, pv);
    }

    return result;
}

void Factory::addPortsFromXML(const QDomElement& element, Node* node) {
    QDomNodeList portsXml = element.elementsByTagName("port");
    for (int i = 0; i < portsXml.size(); ++i ) {
        Port* port = new Port(portsXml.at(i).toElement().attribute("name").toStdString(), node);
        node->addPort(port);
    }
}

Element * Factory::createElementFromXML(const QDomElement & element, const ElementsMap& elementsMap) {
    QString type = element.tagName();
    Element * result = 0;

    if ( type == "link" )
        result = createLink(element, elementsMap);
    else
        result = createNode(element);

    return result;
}

static Element* getElementByName(const QString name, const Factory::ElementsMap& elementsMap) {
    foreach ( QDomElement elem, elementsMap.values() ) {
        if ( elem.attribute("name") == name ) {
            return elementsMap.key(elem);
        }
    }
    return 0;
}

Link * Factory::createLink(const QDomElement & e, const ElementsMap& elementsMap) {
    Element* elem1 = getElementByName(e.attribute("node1"), elementsMap);
    Element* elem2 = getElementByName(e.attribute("node2"), elementsMap);
    if ( elem1 == 0 || elem2 == 0 )
        return 0;

    Link * link = new Link();
    Port* port1 = elem1->toNode()->getPortByName(e.attribute("port1").toStdString());
    Port* port2 = elem2->toNode()->getPortByName(e.attribute("port2").toStdString());
    link->connect(port1, port2);
    port1->connect(link, port2);
    port2->connect(link, port1);
    
    //channel_capacity
    if ( e.hasAttribute("channel_capacity") ) {
	    uint capacity = e.attribute("channel_capacity").toUInt();
	    if ( capacity == 0 )
		    link -> setThroughput ( 10 );
	    else
		    link -> setThroughput ( capacity );
	    
    }
    else link -> setThroughput ( 10 );
    
    //qDebug() << "Channel capacity in link named:" << e.attribute("node1") 
    //<< "<---->" << e.attribute("node2") << " = " << link->getThroughput();
    //

    if ( elem1->isComputer() && elem2->isStore() 
          || elem1->isStore() && elem2->isComputer() ) 
    {
        link->setLatency(Link::AFFINITY);
    } else {
        link->setLatency(Link::NORMAL);
        
    }

    return link;
}

void Factory::setSwitchAttributes(Switch* sw, const QDomElement & e) {
    if ( e.attribute("is_router") == "1" ) {
        sw->attributes |= Switch::ROUTER;
        QDomNodeList services = e.elementsByTagName("service");
        for ( int i = 0; i < services.size(); ++i ) {
            QString service = services.item(i).toElement().attribute("name");
            if ( service == "FW")
                sw->attributes |= Switch::FW;

            if ( service == "DHCP")
                sw->attributes |= Switch::DHCP;

            if ( service == "NAT")
                sw->attributes |= Switch::NAT;

            if ( service == "PAT")
                sw->attributes |= Switch::PAT;
        }
    }
}


Element * Factory::createNode(const QDomElement & e) {
    Element * node = 0;
    QString type = e.tagName();
    Params params = getParametersFromXML(e.elementsByTagName("parameter"));


    if ( type == "vm" || type == "vnf" || type == "server" ) {
        Computer * vm = new Computer(type == "vnf");
        node = ElementFactory::populate(vm, params, type != "server");

        if ( type == "server" && e.attribute("available") == "false" )
            node->setAvailable(false);

    } else if ( type == "st" || type == "storage" ) {
        Store * st = new Store();
        node = ElementFactory::populate(st, params, type == "st");
        setStorageClass(st, e);
    } else if ( type == "netelement" ) {
        Switch * sw = new Switch();

        // switch has additional attributes (is_router, ...)
        setSwitchAttributes(sw, e);
        node = ElementFactory::populate(sw, params);
    } else {
        throw; 
    }

    if ( type == "vm" || type == "vnf" || type == "st" ) {
        setServerLayer((LeafNode *)node, e);
        setDCLayer((LeafNode *)node, e);
    }

    if ( type == "server" || type == "storage" )
        setDCLayer((LeafNode *)node, e);

    if ( node != 0 )
        addPortsFromXML(e, node->toNode());

    return node;
}

void Factory::setServerLayer(LeafNode * node, const QDomElement & e) {
    if ( !e.hasAttribute("sl") )
        return;

    int layer = e.attribute("sl").toInt();
    if ( layer <= 0 || layer > LeafNode::maxLayer() )
        return;

    node->setServerLayer(layer);
}

void Factory::setDCLayer(LeafNode * node, const QDomElement & e) {
    if ( !e.hasAttribute("dl") )
        return;

    int layer = e.attribute("dl").toInt();
    if ( layer <= 0 || layer > LeafNode::maxLayer() )
        return;

    node->setDCLayer(layer);
}

void Factory::setStorageClass(Store * store, const QDomElement & e) {
    if ( !e.hasAttribute("class") )
        return;

    int cl = e.attribute("class").toInt();
    if ( cl <= 0 || cl > 3 ) 
        return;

    store->setClass(cl);
}
