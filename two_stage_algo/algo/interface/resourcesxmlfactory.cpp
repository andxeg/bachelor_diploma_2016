#include "resourcesxmlfactory.h"

#include "factory.h"
#include "network.h"
#include <list>

#include <QString>

ResourcesXMLFactory::ResourcesXMLFactory(const QDomElement & element)
:
	networkXml(element)
{
    QStringList elementTypes;
    elementTypes << "server" << "storage" << "netelement" << "link";
    elementsXML = Factory::getXmlElementsByTypes(elementTypes, networkXml);
    ids = Factory::getReverseIndex(elementsXML);

    std::list<Element*> list = elementsXML.keys().toStdList();
    Elements elements = Elements(list.begin(), list.end());
    for (Elements::iterator it = elements.begin(); it != elements.end(); ++it ) {
        Element * e = *it;
    	e->physical = true;
    }
    
    network = new Network(elements);
    
    //
    if ( element.hasAttribute("migration_time") )
	    network->setMigrationTime( element.attribute("migration_time").toUInt());
    //
}

ResourcesXMLFactory::~ResourcesXMLFactory() {
    foreach( Element* elem, elementsXML.keys() ) {
        delete elem;
    }
    delete network;
}

Network * ResourcesXMLFactory::getNetwork() const {
    return network;
}

QString ResourcesXMLFactory::getName(Element* elem) const {
    QDomElement elemXml = elementsXML.value(elem);
    return elemXml.attribute("name");
}
