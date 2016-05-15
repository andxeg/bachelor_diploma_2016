#include <stdio.h>
#include <vector>
#include "interface/snapshot.h"
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
    if ( argc != 3 )
    {
        printf("Usage: %s <input file> <output file>\n", *argv);
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
    
    algorithm.schedule(); 

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
    
    comment = std::string("After_scheduling");
    snapshot.print(comment);

    if ( nodeAssignedRequests != requests.size())
       return PARTIAL_FAILURE;

    //print current data and time
    time_t t = time(0);
    struct tm * now = localtime(&t);
    std::string result = ToString<int>(now->tm_year + 1900) + std::string("-") +
			ToString<int>(now->tm_mon + 1) + std::string("-") +
			ToString<int>(now->tm_mday ) + std::string(".") +
			ToString<int>(now->tm_hour ) + std::string(":") +
			ToString<int>(now->tm_min ) + std::string(":") +
			ToString<int>(now->tm_sec );
    std::cout << result << std::endl;
    //
    
    return SUCCESS;

}
