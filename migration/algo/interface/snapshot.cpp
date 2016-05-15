#include "snapshot.h"

#include "tenantxmlfactory.h"
#include "resourcesxmlfactory.h"

#include "request.h"
#include "network.h"

#include <QDomElement>
#include <QDomNodeList>

#include <QFile>
#include <QTextStream>

#include <QDebug>
#include <ctime>
#include <sstream>

template <typename T>
std::string ToString(T val)
{
    std::stringstream stream;
    stream << val;
    return stream.str();
}


Snapshot::Snapshot()
:
    network(0)
{
    document = new QDomDocument("XMLDocument");
}

Snapshot::~Snapshot()
{
    delete network;
    foreach(TenantXMLFactory * f, tenants)
        delete f;
    
    delete document;
}

bool Snapshot::read(const QString & filename)
{
    QFile input(filename);
    if ( !input.open(QIODevice::ReadOnly | QIODevice::Text) )
    {
        qDebug() << "Unable to open file" << filename;
        return false;
    }

    QTextStream inputStream(&input);

    QString eMessage;
    int eLine, eColumn;
    if ( !document->setContent(inputStream.readAll(), false, &eMessage, &eLine, &eColumn))
    {
        qDebug() << "XML parsing error, reason:" << eMessage
            << "at line" << eLine << ", column" << eColumn;
        return false;
    }

    QDomElement root = document->documentElement();
    QDomElement resources = root.elementsByTagName("resources").item(0).toElement();
    network = new ResourcesXMLFactory(resources);

    QDomNodeList ts = root.elementsByTagName("tenant");
    QList<TenantXMLFactory *> clients;
    for ( int i = 0; i < ts.size(); i++)
    {
        QDomElement e = ts.item(i).toElement();
        if ( TenantXMLFactory::isProviderTenant(e) )
            continue;
        clients.append(new TenantXMLFactory(e));
    }

    for ( int i = 0; i < ts.size(); i++ )
    {
        QDomElement e = ts.item(i).toElement();
        if ( !TenantXMLFactory::isProviderTenant(e) )
            continue;

        TenantXMLFactory * factory = new TenantXMLFactory(e);
        tenants.insert(factory->name(), factory);
        foreach(TenantXMLFactory * f, clients)
            factory->parseExternalPorts(f->name(), f->getPorts());
    }

    foreach(TenantXMLFactory * client, clients)
        tenants.insert(client->name(), client);
    commit();

    return true;
}

Element * Snapshot::getNetworkElement(const QString & name) const
{
    return network->getElement(name);
}

Element * Snapshot::getTenantElement(const QString & tenant, const QString & name) const
{
    return tenants[tenant]->getElement(name);
}

void Snapshot::commit() 
{
   foreach(TenantXMLFactory * f, tenants)
      f->readAssignmentData(*network);
}

void Snapshot::write(const QString & filename) const
{
    foreach(TenantXMLFactory * f, tenants)
        f->commitPartialAssignmentData(*network);

    QFile output(filename);
    if ( !output.open(QIODevice::WriteOnly | QIODevice::Text) )
        return;

    QTextStream outputStream(&output);
    outputStream << document->toString(4);
}

Network * Snapshot::getNetwork() const 
{
    return network->getNetwork();
}

Requests Snapshot::getRequests() const
{
    Requests result;
    foreach(TenantXMLFactory * f, tenants)
        result.insert(f->getRequest());
    return result;
}


QMap<QString, double> convertMapToQMap(std::map<std::string, double> map) {
	QMap<QString, double> result;
	std::map<std::string, double>::iterator first = map.begin();
	std::map<std::string, double>::iterator last = map.end();
	for ( std::map<std::string, double>::iterator elem = first; elem != last; elem++ ) {
		std::string key = elem->first;
		double value = elem->second;
		QString qkey = QString::fromStdString(key);
		result.insert(qkey, value);
	}
	return result;
}

void Snapshot::print(std::string comment) {

	//Create unique filename
	//std::string directory = std::string("/home/soc/sched_result");
	std::string directory = std::string("./sched_result");
	time_t t = time(0);
	struct tm * now = localtime(&t);
	std::string uuid = ToString<int>(now->tm_year + 1900) + std::string("-") +
				ToString<int>(now->tm_mon + 1) + std::string("-") +
				ToString<int>(now->tm_mday ) + std::string(".") +
				ToString<int>(now->tm_hour ) + std::string(":") +
				ToString<int>(now->tm_min ) + std::string(":") +
				ToString<int>(now->tm_sec ); 
	QString filename = QString::fromStdString( directory + "/" + comment + uuid );
	QFile output(filename);
	if ( !output.open(QIODevice::WriteOnly | QIODevice::Text) )
		qDebug() << "[ERROR] Failed to create a file for the results of the scheduler.";
	QTextStream schedResult(&output);
	//
	
	AssignmentsInfo assignments = getAssignmentsInfo();
	qDebug() << "[SNAPSHOT] Current state:";
	printf("%s\n", comment.c_str());
	schedResult << "[SNAPSHOT] Current state:\n";
    
	foreach ( QString physElem, assignments.keys()) {
        
		printf("\tPhysical element [%s]:\n", physElem.toUtf8().constData());
		schedResult << "\tPhysical element [ " << physElem << "]:\n";
		Element * physicalElement = getNetworkElement(physElem);
		
		QMap<QString, double> physicalParametersTotal = convertMapToQMap( physicalElement->getParametersTotal() );
		QMap<QString, double> physicalParametersUsed = convertMapToQMap( physicalElement->getParametersUsed() );
		
		QMap<QString, double>::iterator firstTotal = physicalParametersTotal.begin();
		QMap<QString, double>::iterator firstUsed = physicalParametersUsed.begin();
		
		printf("\t\t(total)");
		schedResult << "\t\t(total)";
		for ( QMap<QString, double>::iterator param = firstTotal; param != physicalParametersTotal.end(); param++ ) {
			printf(" %s:%1.f", param.key().toUtf8().constData(), param.value());
			schedResult << " " << param.key() << ":" << param.value();
		}
		printf("\n");
		schedResult << "\n";
		
		printf("\t\t(used)");
		schedResult << "\t\t(used)";
		for ( QMap<QString, double>::iterator param = firstUsed; param != physicalParametersUsed.end(); param++ ) {
			printf(" %s:%1.f", param.key().toUtf8().constData(), param.value());
			schedResult << " " << param.key() << ":" << param.value();
		}
		printf("\n");
		schedResult << "\n";
		
		printf("\tVirtual elements:\n");
		schedResult << "\tVirtual elements:\n";
		
		std::vector< std::vector< std::string > > physElemAssignments = assignments[physElem];
		std::vector< std::vector< std::string > >::iterator firstElem = physElemAssignments.begin();
		std::vector< std::vector< std::string > >::iterator lastElem = physElemAssignments.end();
		
		for( std::vector< std::vector < std::string > >::iterator elem = firstElem; elem != lastElem; elem++ ) {
			std::vector < std::string > e = *elem;
			QString tenantName = QString::fromStdString((e[0]));
			QString virtElemName = QString::fromStdString((e[1]));
			Element * virtualElement = getTenantElement(tenantName, virtElemName);
			QMap<QString, double> virtualParameters = convertMapToQMap( virtualElement->getParametersTotal() );
			QMap<QString, double>::iterator first = virtualParameters.begin();
			printf("\t\t");
			schedResult <<"\t\t";
			for ( QMap<QString, double>::iterator param = first; param != virtualParameters.end(); param++ ) {
				printf("%s:%1.f ", param.key().toUtf8().constData(), param.value());
				schedResult << " " << param.key() << ":" << param.value();
			}
			printf("[%s] from tenant [%s]", virtElemName.toUtf8().constData(), tenantName.toUtf8().constData());
			printf("\n");
			schedResult << "[" << virtElemName << "] from tenant [" << tenantName << "]";
			schedResult << "\n";
		}
		printf("\n");
		schedResult << "\n";
    }
}

Snapshot::Assignments Snapshot::parseReverseAssignments() const
{
	Assignments assignments;
	foreach(TenantXMLFactory * f, tenants)
	{
		assignments.insertMulti(f->name(), f->assignments());
	}
	return assignments;
}

Snapshot::AssignmentsInfo Snapshot::getAssignmentsInfo() const  {
    Assignments assignments = parseReverseAssignments();
    AssignmentsInfo result;
    Assignments::iterator first = assignments.begin();
    for ( Assignments::iterator assignmentsElem = first; assignmentsElem != assignments.end(); assignmentsElem++ ) {
        QString tenantName = assignmentsElem.key();
        QMap<QString,QString> virtElemAssignments = assignmentsElem.value();
        QMap<QString,QString>::iterator firstVirtElem = virtElemAssignments.begin();
        for ( QMap<QString,QString>::iterator virtElem = firstVirtElem; virtElem != virtElemAssignments.end(); virtElem++) {
            QString virtElemName = virtElem.key();
            QString physElemName = virtElem.value();
	    std::vector< std::string> pairTenantElem;
	    pairTenantElem.push_back(tenantName.toUtf8().constData());
	    pairTenantElem.push_back(virtElemName.toUtf8().constData());
            result[physElemName].push_back(pairTenantElem);
        }
    }
    return result;
}
