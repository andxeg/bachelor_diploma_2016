#include "snapshot.h"

#include "tenantxmlfactory.h"
#include "resourcesxmlfactory.h"

#include "request.h"
#include "network.h"

#include "leafnode.h"

#include <QDomElement>
#include <QDomNodeList>

#include <QFile>
#include <QTextStream>

#include <QDebug>
#include <ctime>
#include <sstream>

#include <iostream>
#include <fstream>

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
		input.close();
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


	//Read rules
	readTenantsRestrictions();
	//

	//Read all already assigned elements
	commit();
	//

	//Try satisfy VM-PM affinity rules
	tryVmPmAffinity();
	//
	input.close();

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

Snapshot::Assignments Snapshot::assignElements() const
{
	Assignments assignments;
	foreach(TenantXMLFactory * f, tenants) {
			if (f->getRequest()->isAssigned())
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


void Snapshot::readTenantsRestrictions() {
	qDebug() << "readTenantsRestrictions START";
	QDomElement root = document->documentElement();
	QDomElement restrictions = root.elementsByTagName("rules").item(0).toElement();
	QDomElement vm_vm_affinity = restrictions.elementsByTagName("vm_vm_affinity").item(0).toElement();
	QDomElement vm_vm_anti_affinity = restrictions.elementsByTagName("vm_vm_anti_affinity").item(0).toElement();
	QDomElement vm_pm_affinity = restrictions.elementsByTagName("vm_pm_affinity").item(0).toElement();
	QDomElement vm_pm_anti_affinity = restrictions.elementsByTagName("vm_pm_anti_affinity").item(0).toElement();

    int sl = 1;

	qDebug() << "readVmVmAffinity START";
	readVmVmAffinity(vm_vm_affinity, sl);

	qDebug() << "readVmVmAntiAffinity START";
	readVmVmAntiAffinity(vm_vm_anti_affinity, sl);

	qDebug() << "readVmPmAffinity START";
	readVmPmAffinity(vm_pm_affinity);

	qDebug() << "readVmPmAntiAffinity START";
	readVmPmAntiAffinity(vm_pm_anti_affinity);
}

void Snapshot::readVmVmAffinity(const QDomElement & vm_vm_affinity, int & sl) {
	QDomNodeList rules = vm_vm_affinity.elementsByTagName("rule");

	for ( int i = 0; i < rules.size(); i++ ) {

        sl += 1;

		QDomElement rule = rules.item(i).toElement();
		QDomNodeList elems = rule.elementsByTagName("vm");

		//get vector of the Element *
		Elements vms;
		for ( int j = 0; j < elems.size(); j++ ) {
			QDomElement elem = elems.item(j).toElement();
			QString name = elem.attribute("name");
			Element * e = getVirtualElementByName(name);
			if (e) {
                ((LeafNode *)e)->setServerLayer(sl);
                vms.insert(e);
			}
		}

		//save element
		vmVmAffinity.insert(*vms.begin());

		for ( Elements::iterator k = vms.begin(); k != vms.end(); k++ ) {
			Element * vm = *k;
			//append all elements from vms including vm
			//vm should be delete in Element
			Elements vms_copy(vms);
			vm -> addVmVmAffinity(vms_copy);
		}
	}

}

void Snapshot::readVmVmAntiAffinity(const QDomElement & vm_vm_anti_affinity, int & sl) {
	QDomNodeList rules = vm_vm_anti_affinity.elementsByTagName("rule");

	for ( int i = 0; i < rules.size(); i++ ) {

        sl += 1;

        QDomElement rule = rules.item(i).toElement();
		QDomNodeList elems = rule.elementsByTagName("vm");

		//get vector of the virtual Element *
		Elements vms;
		for ( int j = 0; j < elems.size(); j++ ) {
			QDomElement elem = elems.item(j).toElement();
			QString name = elem.attribute("name");
			Element * e = getVirtualElementByName(name);
			if (e) {
                ((LeafNode *)e)->setServerLayer(sl);
                vms.insert(e);
                sl++;
            }
		}

		//save element
		vmVmAntiAffinity.insert(*vms.begin());

		for ( Elements::iterator k = vms.begin(); k != vms.end(); k++ ) {
			Element * vm = *k;
			//append all elements from vms including vm
			//vm should be delete in Element
			Elements vms_copy(vms);
			vm -> addVmVmAntiAffinity(vms_copy);
		}
	}
}

void Snapshot::readVmPmAffinity(const QDomElement & vm_pm_affinity) {
	QDomNodeList rules = vm_pm_affinity.elementsByTagName("rule");

	for ( int i = 0; i < rules.size(); i++ ) {
		QDomElement rule = rules.item(i).toElement();
		QString vm_name = rule.attribute("vm_name");
		QDomNodeList elems = rule.elementsByTagName("pm");

		//get vector of the physical Element *
		Elements pms;
		for ( int j = 0; j < elems.size(); j++ ) {
			QDomElement elem = elems.item(i).toElement();
			QString name = elem.attribute("name");
			Element * e = getNetworkElement(name);
			if (e)
				pms.insert(e);
		}

		//
		Element * virtElem = getVirtualElementByName(vm_name);
		if (!virtElem)
			break;

		virtElem->addVmPmAffinity(pms);
		//save virtElem
		vmPmAffinity.insert(virtElem);

	}
}

void Snapshot::readVmPmAntiAffinity(const QDomElement & vm_pm_anti_affinity) {
	QDomNodeList rules = vm_pm_anti_affinity.elementsByTagName("rule");

	for ( int i = 0; i < rules.size(); i++ ) {
		QDomElement rule = rules.item(i).toElement();
		QString vm_name = rule.attribute("vm_name");
		QDomNodeList elems = rule.elementsByTagName("pm");

		//get vector of the physical Element *
		Elements pms;
		for ( int j = 0; j < elems.size(); j++ ) {
			QDomElement elem = elems.item(i).toElement();
			QString name = elem.attribute("name");
			Element * e = getNetworkElement(name);
			if (e)
				pms.insert(e);
		}

		//
		Element * virtElem = getVirtualElementByName(vm_name);
		if (!virtElem)
			break;

		virtElem->addVmPmAntiAffinity(pms);
		//save virtElem
		vmPmAntiAffinity.insert(virtElem);

	}
}

Element * Snapshot::getVirtualElementByName(const QString & name ) {
	Element * virtElem = NULL;
	QMap<QString, TenantXMLFactory *>::iterator first = tenants.begin();
	QMap<QString, TenantXMLFactory *>::iterator last = tenants.end();
	for (QMap<QString, TenantXMLFactory *>::iterator tenant = first; tenant != last; tenant++ ) {
		virtElem = getTenantElement(tenant.key(), name);
		if (virtElem)
			break;
	}

	return virtElem;
}


void Snapshot::tryVmPmAffinity() {
	Elements::iterator first = vmPmAffinity.begin();
	Elements::iterator last = vmPmAffinity.end();

	for ( Elements::iterator i = first; i != last; i++ ) {
		//try assign vm on pm
		Element * vm = (*i);

		Elements tmp = vm->vm_pm_affinity;
		std::vector<Element *> pms;
		pms.insert(pms.end(), tmp.begin(), tmp.end());

		std::sort(pms.begin(), pms.end(), Criteria::elementWeightAscending);

		bool flag = false;

		for (size_t k = 0; k < pms.size(); k++) {
			Element * pm = pms[k];
			if ( pm->assign(vm)) {
				//for this vm migration label == 0
				((LeafNode *)vm)->setMigration(0);
				flag = true;
				break;
			}
		}

		if (!flag)
			qDebug() << "vm_pm_affinity error";
	}
}

void Snapshot::printRulesStat() {
	size_t correct = 0;//satisfy rules
	size_t unassigned = 0;
	size_t incorrect = 0;//does not satisfy rules

	//
	printf("VM-PM affinity rules:\n");
	printf("All -> %lu\n", vmPmAffinity.size());
	for (Elements::iterator i = vmPmAffinity.begin(); i != vmPmAffinity.end(); i++ ) {
		Element * vm = (*i);
		if (!vm->isAssigned()) {
			unassigned += 1;
			continue;
		}

		Element * pm = vm->getAssignee();
		Elements::iterator a = vm->vm_pm_affinity.find(pm);
		if ( a != vm->vm_pm_affinity.end() ) {
			correct += 1;
			continue;
		}
		incorrect += 1;
	}

	printf("Correct -> %zu\n", correct);
	printf("Unassigned -> %zu\n", unassigned);
	printf("Incorrect -> %zu\n", incorrect);
	//
	printf("\n");
	correct = unassigned = incorrect = 0;
	//
	printf("VM-PM anti-affinity rules:\n");
	printf("All -> %lu\n", vmPmAntiAffinity.size());
	for (Elements::iterator i = vmPmAntiAffinity.begin(); i != vmPmAntiAffinity.end(); i++ ) {
		Element *vm = (*i);
		if (!vm->isAssigned()) {
			unassigned += 1;
			continue;
		}

		Element *pm = vm->getAssignee();
		Elements::iterator a = vm->vm_pm_anti_affinity.find(pm);
		if (a == vm->vm_pm_anti_affinity.end()) {
			correct += 1;
			continue;
		}
		incorrect += 1;
	}

	printf("Correct -> %zu\n", correct);
	printf("Unassigned -> %zu\n", unassigned);
	printf("Incorrect -> %zu\n", incorrect);
	//
	printf("\n");
	correct = unassigned = incorrect = 0;
	//
	printf("VM-VM affinity rules:\n");
	printf("All -> %lu\n", vmVmAffinity.size());
	for (Elements::iterator i = vmVmAffinity.begin(); i != vmVmAffinity.end(); i++ ) {
		Element * vm = (*i);
		Elements pms;
		if (vm->isAssigned())
			pms.insert(vm->getAssignee());

		bool unassign = false;

		for (Elements::iterator j = vm-> vm_vm_affinity.begin(); j != vm->vm_vm_affinity.end(); j++ ){
			Element * other_vm = (*j);
			if (other_vm->isAssigned())
				pms.insert(other_vm->getAssignee());
			if (pms.size() >= 2 ) {
				unassign = true;
				break;
			}
		}

		if (unassign)
			unassigned += 1;
		else
			correct += 1;
	}
	printf("Correct -> %zu\n", correct);
	printf("Unassigned -> %zu\n", unassigned);
	//
	printf("\n");
	correct = unassigned = incorrect = 0;
	//
	printf("VM-VM anti-affinity rules:\n");
	printf("All -> %lu\n", vmVmAntiAffinity.size());
	for (Elements::iterator i = vmVmAntiAffinity.begin(); i != vmVmAntiAffinity.end(); i++ ) {
		Element * vm = (*i);
		Elements pms;
		if (vm->isAssigned())
			pms.insert(vm->getAssignee());

		bool unassign = false;

		for (Elements::iterator j = vm-> vm_vm_anti_affinity.begin(); j != vm->vm_vm_anti_affinity.end(); j++ ){
			Element * other_vm = (*j);
			if (other_vm->isAssigned()) {
				Elements::iterator a = pms.find(other_vm->getAssignee());
				if ( a != pms.end() ){
					unassign = true;
					break;
				}
				pms.insert(other_vm->getAssignee());
			}
		}

		if (!unassign && pms.size() >= 2)
			correct += 1;
		else
			unassigned += 1;

	}
	printf("Correct -> %zu\n", correct);
	printf("Incorrect -> %zu\n", unassigned);
}


void Snapshot::printResultsInHuaweiStyle(const std::string & filename) const {

	foreach(TenantXMLFactory * f, tenants)
		f->commitPartialAssignmentData(*network);

	std::ofstream fout;
	fout.open(filename.c_str(), std::ios_base::out | std::ios_base::trunc);
	if ( !fout.is_open()) {
		std::cout << "Error in openning output file" << std::endl;
		return;
	}

	size_t succAssigned = 0;

	QMap<QString, TenantXMLFactory *>::const_iterator firstTenant = tenants.begin();
	QMap<QString, TenantXMLFactory *>::const_iterator lastTenant = tenants.end();
	for (QMap<QString, TenantXMLFactory *>::const_iterator i = firstTenant; i != lastTenant; i++) {
		Request * tenant = (*i)->getRequest();
		if (tenant->isAssigned())
			succAssigned++;
	}

	//Print number of tenants assigned successfully
	fout << succAssigned << std::endl;

	//Print info about tenants
	Assignments assignments = assignElements();
	printf("\n\nAssignment size -> %d\n\n", assignments.size());
	Assignments::iterator first = assignments.begin();
	for ( Assignments::iterator assignmentsElem = first; assignmentsElem != assignments.end(); assignmentsElem++ ) {
		QString tenantName = assignmentsElem.key();

		//Print index of tenant - its name
		fout << tenantName.toUtf8().constData() << std::endl;

		//Print VMs assignment info: <index_of_vm> <assigned_pm>
		QMap<QString,QString> virtElemAssignments = assignmentsElem.value();
		QMap<QString,QString>::iterator firstVirtElem = virtElemAssignments.begin();
		//std::cout << "1" << std::endl;
		for ( QMap<QString,QString>::iterator virtElem = firstVirtElem; virtElem != virtElemAssignments.end(); virtElem++) {
			QString virtElemName = virtElem.key();
			QString physElemName = virtElem.value();
			fout << virtElemName.toUtf8().constData() << ' ' << physElemName.toUtf8().constData() << std::endl;
		}

		//Print LINKs assignment info: <start_node> <end_node> <num_of_phys_nodes> <node1>...<nodeN>
		LinkAssignments linkAssignments = getLinkAssignments();
		std::vector<std::string> links = linkAssignments[tenantName];
		for (size_t i = 0; i < links.size(); i++) {
			fout << links[i] << std::endl;
		}
	}
	//

	//
//	int value;
//	std::string key = ToString<int>(value);
//	QString qkey = QString::fromStdString(key);
//	//

}


Snapshot::LinkAssignments Snapshot::getLinkAssignments() const {
	LinkAssignments result;
	foreach(TenantXMLFactory * f, tenants) {
			QString tenantName = f->name();
			Request * tenant = f->getRequest();
			Elements links = tenant->getTunnels();
			std::vector<std::string> linksInfo;
			for (Elements::iterator i = links.begin(); i != links.end(); i++) {
				Element * link = (*i);
				Path route = link->toLink()->getRoute();

				Element * firstVirtElem = link->toEdge()->getFirst()->getParentNode();
				Element * secondVirtElem = link->toEdge()->getSecond()->getParentNode();;
				std::string first;
				std::string second;
				Factory::IDS ids = f->ids;

				for (QMap<QString, Element *>::iterator j = ids.begin(); j != ids.end(); j++ ) {
					if (j.value() == firstVirtElem )
						first = j.key().toUtf8().constData();
					else if (j.value() == secondVirtElem )
						second = j.key().toUtf8().constData();
				}

				linksInfo.push_back( first
									+ std::string(" ") +
										second
									+ std::string(" ") +
										getPathInHuaweiStyle(route, *network));
			}
			result[tenantName] = linksInfo;
	}

	return result;
}


std::string Snapshot::getPathInHuaweiStyle(Path& route, const ResourcesXMLFactory & resourceFactory) const {
	std::stringstream linkInfo;
	std::vector<Element *> path = route.getPath();

	if (path.size() == 0)
		return linkInfo.str();

	//minimum one edge exist
	//add first edge
	linkInfo << resourceFactory.getName(path.front()->toEdge()->getFirst()->getParentNode()).toUtf8().constData() << ' ';
	linkInfo << resourceFactory.getName(path.front()->toEdge()->getSecond()->getParentNode()).toUtf8().constData() << ' ';

	Element * prevNode = path.front()->toEdge()->getSecond()->getParentNode();
	size_t length = 0;

	for (std::vector<Element *>::iterator it = ++path.begin(); it != path.end(); ++it) {

		Element *e = *it;
		if ( !e->isEdge() )
			continue;
		Port* port1 = e->toEdge()->getFirst();
		Port* port2 = e->toEdge()->getSecond();
		if (port1->getParentNode() == prevNode) {
			linkInfo << resourceFactory.getName(port2->getParentNode()).toUtf8().constData() << ' ';
			length++;
			prevNode = port2->getParentNode();
		} else if (port2->getParentNode() == prevNode) {
			linkInfo << resourceFactory.getName(port1->getParentNode()).toUtf8().constData() << ' ';
			length++;
			prevNode = port1->getParentNode();
		}
	}

	std::stringstream tmp;
	tmp <<  length + 2 << ' ' << linkInfo.str();

	return tmp.str();
}
