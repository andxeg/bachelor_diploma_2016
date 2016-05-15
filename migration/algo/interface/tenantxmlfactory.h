#pragma once

#include "defs.h"
#include "factory.h"

class QDomElement;
class NetworkXMLFactory;

//
class Snapshot;
//

class TenantXMLFactory {
	//
	friend class Snapshot;
	//
public:
    TenantXMLFactory(const QDomElement & element);
    virtual ~TenantXMLFactory();
    Request * getRequest() const;
    void readAssignmentData(const class ResourcesXMLFactory& resourceFactory);
    //
    void readAssignmentDataAttributes( Element * element);
    //
    void commitAssignmentData(const class ResourcesXMLFactory& resourceFactory);
    void commitPartialAssignmentData(const class ResourcesXMLFactory & resourceFactory);

    // Get string representation for path in the form "NodeName: PortName; NodeName:PortName ..."
    QString getPathXml(class Path& path, const class ResourcesXMLFactory & resourceFactory) const;
    QString getPhysicalPortXML(Port * port, const class ResourcesXMLFactory & rf) const;

    static bool isProviderTenant(const QDomElement & element);

    // Parse external ports
    void parseExternalPorts(QString clientName, Ports ports);
    QString name() const;
    QMap<QString, QString> assignments() const;
    Element * getElement(const QString & name) const { return ids[name]; }

    inline const Ports& getPorts() {
        return ports;
    }
private:

    // Add external ports from vnf
    void addExternalPorts(Node* elem);
    void assignLink(Link * link, const class ResourcesXMLFactory & resourceFactory);

    // Method to check whether the element is non-router net-element
    bool isNonRouterSwitch(const Element* elem);

    QDomElement generateLinkElement(Link * link);
private:
    Request * request;
    QDomElement tenant;
    Factory::ElementsMap elementsXML;
    Factory::IDS ids;

    Ports ports; // all existing ports
    QMap<QString, std::set<std::pair<Element*, QString> > > externalPorts; // external port name with client's name
};
