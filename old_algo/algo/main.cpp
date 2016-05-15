#include <stdio.h>
#include <vector>
#include "interface/snapshot.h"
#include "interface/elementfactory.h"
#include "test/testalgorithm.h"
#include "prototype/prototype.h"


#include "defs.h"
#include "request.h"
#include "network.h"
#include "operation.h"
#include "criteria.h"

#include <QString>
#include <ctime>
#include <iostream>
#include <sstream>

template <typename T>
std::string ToString(T val)
{
    std::stringstream stream;
    stream << val;
    return stream.str();
}

enum ReturnCodes {
   SUCCESS = 0,
   PARTIAL_FAILURE,
   INVALID_INPUT,
   INVALID_USAGE
};

int main(int argc, char ** argv)
{
    //print current data and time
    time_t t = time(0);
    struct tm * now = localtime(&t);
    std::string result = ToString<int>(now->tm_year + 1900) + std::string("-") +
                         ToString<int>(now->tm_mon + 1) + std::string("-") +
                         ToString<int>(now->tm_mday ) + std::string(".") +
                         ToString<int>(now->tm_hour ) + std::string(":") +
                         ToString<int>(now->tm_min ) + std::string(":") +
                         ToString<int>(now->tm_sec );
    std::cout << "Current time -> " << result << std::endl;
    //
    int hour_start = now->tm_hour;
    int min_start = now->tm_min;
    int sec_start = now->tm_sec;
    //

    if ( argc != 4 )
    {
        printf("Usage: %s <input file> <output file xml> <output file huawei>\n", *argv);
        return INVALID_USAGE;
    }

    Snapshot snapshot;
    if ( !snapshot.read(argv[1]) )
       return INVALID_INPUT;

    std::string comment = std::string("Before_scheduling");
    snapshot.print(comment);

    Requests requests = snapshot.getRequests();
    PrototypeAlgorithm algorithm(snapshot.getNetwork(), requests);
    
    //
    algorithm.setResources(snapshot.getResources());
    algorithm.setTenants(snapshot.getTenants());
    //


    //Set to all virtual links capacity = 0;
//    for (Requests::iterator r = requests.begin(); r != requests.end(); r++) {
//        Request * request = *r;
//        Elements tunnels = request->getTunnels();
//        for (Elements::iterator k = tunnels.begin(); k != tunnels.end(); k++) {
//            (*k)->toLink()->setThroughput(0);
//        }
//    }
    //

    std::cout << "BEFORE algorithm.schedule();" << std::endl;
    //algorithm.oneStageSchedule();
    //algorithm.twoStageSchedule();
    algorithm.oldAlgorithm();
    std::cout << "AFTER algorithm.schedule();" << std::endl;

    int assignedRequests = 0;
    int nodeAssignedRequests = 0;
    for ( Requests::iterator i = requests.begin(); i != requests.end(); i++ ) {
        Request * r = *i;
        if ( r->isAssigned() )
            assignedRequests++;    
        Elements unassignedComputational = Operation::filter(r->elementsToAssign(), Criteria::isComputational);
        if ( unassignedComputational.empty() )
            nodeAssignedRequests++;
    }

    snapshot.write(argv[2]);
    std::cout << "Before printResultsInHuaweiStyle" << std::endl;
    snapshot.printResultsInHuaweiStyle(argv[3]);

    comment = std::string("After_scheduling");
    //snapshot.print(comment);

    //print current data and time
    t = time(0);
    now = localtime(&t);
    result = ToString<int>(now->tm_year + 1900) + std::string("-") +
             ToString<int>(now->tm_mon + 1) + std::string("-") +
             ToString<int>(now->tm_mday ) + std::string(".") +
             ToString<int>(now->tm_hour ) + std::string(":") +
             ToString<int>(now->tm_min ) + std::string(":") +
             ToString<int>(now->tm_sec );
    std::cout << "Current time -> " << result << std::endl;
    //
    int time = now->tm_hour * 3600 + now->tm_min * 60 + now->tm_sec - hour_start * 3600 - min_start * 60 - sec_start;
    std::cout << "Algorithm work time -> " <<  time / 3600  << "  hours " << (time % 3600) / 60  << " mins " << (time % 3600) % 60 << " secs" << std::endl;
    //

    {
        std::cout << std::endl;
        std::cout << "All virtual resources ";
        //Virtual machines
        
        //std::cout <<  (*requests.begin())->getName() << std::endl;
        
        std::map<std::string, double> vmParams = (*requests.begin())->getAllVMsParameters();
        Requests::iterator firstReq = ++requests.begin();
        Requests::iterator lastReq = requests.end();
        for (Requests::iterator i = firstReq; i != lastReq; i++) {
            Request *request = *i;
            //std::cout <<  request->getName() << std::endl;
            std::map<std::string, double> reqParams = request->getAllVMsParameters();

            std::map<std::string, double>::iterator f = reqParams.begin();
            std::map<std::string, double>::iterator l = reqParams.end();
            for (std::map<std::string, double>::iterator j = f; j != l; j++) {
                std::string paramName = j->first;
                double paramValue = j->second;
                vmParams[paramName] += paramValue;
            }

        }

        std::map<std::string, double>::iterator first = vmParams.begin();
        std::map<std::string, double>::iterator last = vmParams.end();
        for (std::map<std::string, double>::iterator i = first; i != last; i++) {
            std::cout << i->first << ' ' << i->second <<' ';
        }

        //Print all link's information
        std::cout << std::endl;
        double sumThroughput = 0.0;
        for (Requests::iterator k = requests.begin(); k != requests.end(); k++) {
            sumThroughput += (*k)->getAllLinksParameters();
        }
        std::cout << "Sum virtual throughput " << sumThroughput << std::endl;

        //TODO:
        //print  virtual storages resources
        //print virtual links resources
//        std::cout << std::endl;
//        std::cout << "Virtual links" << std::endl;
//        for (Requests::iterator i = requests.begin(); i != requests.end(); i++) {
//            Request * request = (*i);
//            Elements links = request->getTunnels();
//            std::cout << "Tenant: " << request->getName() << std::endl;
//            for (Elements::iterator j = links.begin(); j != links.end(); j++) {
//                Link * link = (*j)->toLink();
//                Elements neighbors = (*j)->toEdge()->adjacentNodes();
//                std::vector<Element *> path = link->getRoute().getPath();
//                std::cout << "Tunnel: " << link << " " << link->getThroughput() <<
//                " between " <<  snapshot.getTenants()[*neighbors.begin()][1] << " and " <<
//                snapshot.getTenants()[*(++neighbors.begin())][1] << std::endl;
//                for (size_t k = 0; k != path.size(); k++) {
//                    if (path[k]->isEdge()) {
//                        Elements adjacents = path[k]->adjacentNodes();
//                        std::cout << "\t\tPhysical link between " <<
//                                snapshot.getPhysicalElementByName(*adjacents.begin()) << " and " <<
//                                snapshot.getPhysicalElementByName(*(++adjacents.begin())) <<
//                                " with throughput " <<  path[k]->toLink()->getThroughput() << " and " <<
//                                " full throughput " << path[k]->toLink()->getFullThroughput() << std::endl;
//                    }
//                }
//                std::cout << std::endl;
//            }
//        }

    }

    std::cout << std::endl;
    std::cout << "All physical resources" << ' ';
    //Servers
    std::map<std::string, double> allPhysicalParams = snapshot.getNetwork()->getServersParameters();
    std::map<std::string, double>::iterator first = allPhysicalParams.begin();
    std::map<std::string, double>::iterator last = allPhysicalParams.end();
    for (std::map<std::string, double>::iterator i = first; i != last; i++ ){
        std::cout << i->first << ' ' << i->second << ' ';
    }

    //Print all link's information
    std::cout << std::endl;
    double sumThroughput = 0.0;
    sumThroughput = snapshot.getNetwork()->getLinksParameters();
    std::cout << "Sum physical throughput " << sumThroughput << std::endl;

    //TODO:
    //print storages resources
    //print links resources
//    std::cout << std::endl;
//    Network * network = snapshot.getNetwork();
//    Elements links = network->getLinks();
//    for (Elements::iterator i = links.begin(); i != links.end(); i++) {
//        Edge * edge = (*i)->toEdge();
//        Elements adjacents = edge->adjacentNodes();
//        std::cout << "\t\tPhysical link between " <<
//            snapshot.getPhysicalElementByName(*adjacents.begin()) << " and " <<
//            snapshot.getPhysicalElementByName(*(++adjacents.begin())) <<
//            " with throughput " <<  edge->toLink()->getThroughput() << " and " <<
//            " full throughput " << edge->toLink()->getFullThroughput() << std::endl;
//    }
    //

    //TODO:
    //virtual resources assigned successfully
    //and utilization rate of physical resources
    std::cout << std::endl;
    std::cout << "Used physical resources" << ' ';
    //Servers
    std::map<std::string, double> usedPhysicalParams = snapshot.getNetwork()->getUsedServersParameters();
    std::map<std::string, double>::iterator firstParam = usedPhysicalParams.begin();
    std::map<std::string, double>::iterator lastParam = usedPhysicalParams.end();
    for (std::map<std::string, double>::iterator i = firstParam; i != lastParam; i++ ){
        std::cout << i->first << ' ' << i->second << ' ';
    }

    //Print all link's information
    std::cout << std::endl;
    double sumUsedThroughput = snapshot.getNetwork()->getUsedLinksParameters();
    std::cout << "Used physical throughput  " << sumUsedThroughput << std::endl;
    //

    //utilization of physical resources
    std::cout << std::endl;
    for (std::map<std::string, double>::iterator virt = usedPhysicalParams.begin(), phys = allPhysicalParams.begin();
            virt != usedPhysicalParams.end() || phys != allPhysicalParams.end(); virt++, phys++) {
        std::cout << virt->first << " utilization " << virt->second / phys->second << ' ';
    }
    //Network resources utilization
    std::cout << "Network utilization " << sumUsedThroughput / sumThroughput << std::endl;
    //

    std::cout << std::endl;
    if ( nodeAssignedRequests != requests.size()) {
        printf("Requests all -> %lu ", requests.size());
        printf("Requests assigned -> %u\n", assignedRequests);
        printf("\nPrint rules statistics\n");
        snapshot.printRulesStat();
        ElementFactory::deleteParameters();
        return PARTIAL_FAILURE;
    }

    printf("Requests all -> %lu ", requests.size());
    printf("Requests assigned -> %u\n", assignedRequests);
    printf("\nPrint rules statistics\n");
    snapshot.printRulesStat();
    ElementFactory::deleteParameters();
    return SUCCESS;

}
