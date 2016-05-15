#pragma once

#include "defs.h"

#include <QList>
#include <QMap>

//
#include "network.h"
#include "tenantxmlfactory.h"
#include "resourcesxmlfactory.h"
#include <string>
//

class ResourcesXMLFactory;
class TenantXMLFactory;
class QDomDocument;

class Snapshot {
public:
    typedef QMap<QString, QMap<QString, QString> > Assignments;
    typedef QMap<QString, std::vector< std::vector< std::string> > > AssignmentsInfo;
public:
    Snapshot();
    ~Snapshot();

    bool read(const QString & filename);
    void write(const QString & filename) const;

    Network * getNetwork() const;
    Requests getRequests() const;
    Assignments parseReverseAssignments() const; 
    AssignmentsInfo getAssignmentsInfo() const;

    void print(std::string comment);

    Element * getNetworkElement(const QString & name) const;
    Element * getTenantElement(const QString & tenant, const QString & name) const;
    
    //
    Resources getResources() {
	    Resources result;
	    Network * resources = getNetwork();
	    Elements physElements = resources->getElements();
	    for ( Elements::iterator i=physElements.begin(); i!=physElements.end(); i++) {
		    std::string resourceName = network->getName(*i).toUtf8().constData();
		    result[*i] = resourceName; 
	    }
	    return result;
    }
    
    TenantsElements getTenants() { 
	TenantsElements result;
	QMap<QString, TenantXMLFactory *>::iterator tenant = tenants.begin();
	for ( ;tenant != tenants.end(); tenant++) {
		std::string tenantName = tenant.value()->name().toUtf8().constData();;
		QMap<QString, Element *> tenantElements = tenant.value()->ids;
		QMap<QString, Element *>::iterator tenantElement = tenantElements.begin();
		for ( ;tenantElement != tenantElements.end(); tenantElement++) {
			std::vector < std::string > names;
			std::string tenantElementName = tenantElement.key().toUtf8().constData();
			names.push_back(tenantName);
			names.push_back(tenantElementName);
			result[tenantElement.value()] =  names;
		}
	}
	return result;
    }
    //
    
private:
    void commit();
private:
    QDomDocument * document;
    ResourcesXMLFactory * network;
    QMap<QString, TenantXMLFactory *> tenants;
};
