#include "tenantxmlfactory.h"
#include "resourcesxmlfactory.h"

#include "factory.h"
#include "request.h"
#include "port.h"
#include "node.h"
#include "link.h"
#include "switch.h"
#include <list>
#include <QDebug>

//
#include "leafnode.h"
//

TenantXMLFactory::TenantXMLFactory(const QDomElement & element) 
:
    tenant(element)
{
    QStringList elementTypes;
    elementTypes << "vm" << "st" << "netelement" << "vnf" << "domain" << "link";
    elementsXML = Factory::getXmlElementsByTypes(elementTypes, tenant);
    ids = Factory::getReverseIndex(elementsXML);

    std::list<Element*> list = elementsXML.keys().toStdList();

    // Parsing elements, removing extra net-elements, saving ports and external ports
    Elements elements;

    for ( std::list<Element*>::iterator it = list.begin(); it != list.end(); ++it ) {
    	Element* elem = (*it);

        if ( elem->isNode() )
            ports.insert(elem->toNode()->ports.begin(), elem->toNode()->ports.end());

        if ( elem->isVnf() )
            addExternalPorts(elem->toNode());

        elements.insert(elem);
    }

    request = new Request(elements, name().toStdString());
    if ( element.hasAttribute("dl") )
       request->affine = true;
    if ( isProviderTenant(element) )
       request->provider = true;
}

TenantXMLFactory::~TenantXMLFactory() {
    foreach( Element* elem, elementsXML.keys() ) {
        delete elem;
    }
    delete request;
}

bool TenantXMLFactory::isNonRouterSwitch(const Element* elem) {
    return elem->isSwitch() && (elem->attributes & Switch::ROUTER) == 0;
}

Request * TenantXMLFactory::getRequest() const {
    return request;
}

static void setPortAssignee(QDomNodeList portsXml, QString portName, QString assigneeName) {
    for ( int i = 0; i < portsXml.size(); ++i ) {
        if (portsXml.item(i).toElement().attribute("name") == portName)
            portsXml.item(i).toElement().setAttribute("assignedTo", assigneeName);
    }
}

void TenantXMLFactory::commitPartialAssignmentData(const ResourcesXMLFactory& resourceFactory) {
    Elements elements =  request->getElements();
    for ( Elements::iterator it = elements.begin(); it != elements.end(); ++it ) {
        Element * e = *it;

        if ( !e->isAssigned() )
            continue;

        // check whether this is virtual link first and it is assigned
        if ( e->isLink() ) {
            assignLink(e->toLink(), resourceFactory);
        } else {
            elementsXML[e].setAttribute("assignedTo", resourceFactory.getName(e->getAssignee()));
        }
        // assign ports
        if ( !e->isNode() )
            continue;

        Ports ports = (*it)->toNode()->getPorts();
        for ( Ports::const_iterator pit = ports.begin(); pit != ports.end(); ++pit ) {
            Port * p = *pit;
            if ( p->getAssignee() == 0 )
                continue;

            QString portName = QString::fromUtf8((*pit)->getName().c_str());
            QString assigneeName = QString::fromUtf8((*pit)->getAssignee()->getName().c_str());
            setPortAssignee(elementsXML[*it].elementsByTagName("port"), portName, assigneeName);
        }
    }
}


//if ( migration == 1 ) => migration is  allowed
/*There are three variants with correctly attribute values
 * 1 -> activity set, migration set and label == 0 or 1
 * 2 -> activity set, migration is not set ( default_migration == 1 )
 * 3 -> activity is not set ( default_activity == 0 ), migration is not set ( default_migration == 1 )
 * 4 -> activity is not set ( default_activity == 0 ), migration set and label = 0 or 1
 * All other variants are incorrect
 */
void TenantXMLFactory::readAssignmentDataAttributes( Element * element) {
	
	if ( !element -> isComputer() && !element -> isStore() ) return;
	
	QDomElement elemXML = elementsXML[element];
	LeafNode * node = (LeafNode *)element;
	
	//qDebug() << "Tenant name -> " << name() << "Element -> " << elemXML.attribute("name");
	
	if ( elemXML.hasAttribute("activity") ) {
		//qDebug() << "Hello from " << elemXML.attribute("name") ;
		uint activity = elemXML.attribute("activity").toUInt();
		if ( activity != 0 )
			node -> setActivity(activity);
		//qDebug() << "activity = " << node->getActivity();
	}
	
	if ( elemXML.hasAttribute("migration") ) {
		uint label = elemXML.attribute("migration").toUInt();
		//qDebug() << "Migration label -> " << label;
		if ( label > 1 ) return;
		node -> setMigration(label);
	}
}
//



void TenantXMLFactory::readAssignmentData(const ResourcesXMLFactory& resourceFactory) {
    QMap<QString, QString> data = assignments();
    foreach(QString a, data.keys())  {
        Element * assignment = getElement(a);
        Element * assignee = resourceFactory.getElement(data[a]);
        if ( assignee->assign(assignment) ) {
		readAssignmentDataAttributes( assignment );
		continue;
	}

        qDebug() << "Was unable to assign" << a << "to" << data[a];
    }
}

void TenantXMLFactory::assignLink(Link * link, const ResourcesXMLFactory & resourceFactory) {
    if ( !link->isAssigned() )
        return;

    QDomElement element = elementsXML[link];
    if ( element.isNull() )
        element = generateLinkElement(link);

    Path route = link->getRoute();
    element.setAttribute("assignedTo", getPathXml(route, resourceFactory));
}

QDomElement TenantXMLFactory::generateLinkElement(Link * link) {
    QDomDocument document = tenant.ownerDocument();
    QDomElement result = document.createElement("link");
    QDomNodeList tmp = tenant.elementsByTagName("list_of_links");
    QDomNode list_of_links;
    if ( tmp.size() != 0 ) {
        list_of_links = tmp.at(0);
    } else {
        list_of_links = document.createElement("list_of_links");
        tenant.appendChild(list_of_links);
    }

    list_of_links.appendChild(result);
        
    return result;
}

void TenantXMLFactory::commitAssignmentData(const ResourcesXMLFactory& resourceFactory) {
    if ( !request->isAssigned() )
        return;

    commitPartialAssignmentData(resourceFactory);

}

QString TenantXMLFactory::getPathXml(Path& route, const ResourcesXMLFactory & resourceFactory) const {
    std::vector<Element *> path = route.getPath();
    QStringList unjoinedResult;

    for (std::vector<Element *>::iterator it = path.begin(); it != path.end(); ++it) {
        Element *e = *it;
        if ( !e->isEdge() )
            continue;
        Port* port1 = e->toEdge()->getFirst();
        Port* port2 = e->toEdge()->getSecond();

        unjoinedResult << getPhysicalPortXML(port1, resourceFactory)
            << getPhysicalPortXML(port2, resourceFactory);
    }

    return unjoinedResult.join("; ");
}

QString TenantXMLFactory::getPhysicalPortXML(Port * port, const ResourcesXMLFactory & rf) const {
    return QString("%1:%2")
        .arg(rf.getName(port->getParentNode()))
        .arg(QString::fromStdString(port->getName()));
}

void TenantXMLFactory::parseExternalPorts(QString clientName, Ports ports) {
    if ( externalPorts.find(clientName) != externalPorts.end() ) {
        std::set<std::pair<Element*, QString> > clientInfo = externalPorts.value(clientName);
        std::set<std::pair<Element*, QString> >::iterator it = clientInfo.begin();
        for ( ; it != clientInfo.end(); ++it ) {
            QString name = it->second;
            Node * vnf = it->first->toNode();
            // searching the port in client
            for ( Ports::iterator pit = ports.begin(); pit != ports.end(); ++pit ) {
                std::string stdName = name.toUtf8().constData();
                if ( (*pit)->getName().compare(stdName) == 0 ) {
                    // searching for the free port in our vnf
                    Port* freePort = vnf->getFreePort();
                    if ( freePort == 0 ) {
                        qDebug() << "Cannot create external link: no free port available in vnf!\n";
                        break;
                    }

                    Link* externalLink = new Link();
                    externalLink->connect(freePort, *pit);
                    (*pit)->connect(externalLink, freePort);
                    freePort->connect(externalLink, *pit);
                    qDebug() << "External link created from " << QString::fromStdString((*pit)->getName())
                        << " of " << QString::fromStdString(this->request->getName())
                        << " to " << QString::fromStdString(freePort->getName())
                        << " of " << clientName;
                    request->addExternalLink(externalLink);
                }
            }
        }
    }
}

QString TenantXMLFactory::name() const
{
    return tenant.attribute("name");
}

QMap<QString, QString> TenantXMLFactory::assignments() const
{
    QMap<QString, QString> result;
    foreach(QDomElement e, elementsXML)
    {
        QString assignee = e.attribute("assignedTo");
        if ( assignee.isEmpty() )
            continue;

        QString assignment = e.attribute("name");
        if ( assignment.isEmpty() )
            continue;

        result.insert(e.attribute("name"), assignee);
    }
    return result;
}

bool TenantXMLFactory::isProviderTenant(const QDomElement & element) {
	return element.elementsByTagName("vnf").length() > 0;
}

void TenantXMLFactory::addExternalPorts(Node* elem) {
    QDomElement elemXml = elementsXML.value(elem);

    QString clientName = elemXml.attribute("user_name");
    if ( clientName.length() == 0 ) {
        qDebug() << "Client not found\n";
        return;
    }

    QDomNodeList portsXml = elemXml.elementsByTagName("external_port");
    for (int i = 0; i < portsXml.size(); ++i ) {
        QString name = portsXml.at(i).toElement().attribute("name");
        if ( name.length() > 0 ) {
            externalPorts[clientName].insert(std::pair<Element*, QString>(elem, name));
        }
    }
}
