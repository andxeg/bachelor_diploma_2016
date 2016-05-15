#pragma once

#include "graph.h"
#include "element.h"
#include "operation.h"
#include "criteria.h"

class Network : public Graph {
public:
    Network(const Elements & e) {
        elements = Operation::filter(e, Criteria::isPhysical);
	//
	migrationTime = 100;
	//
    }

    inline Elements availableElements() const {
        return Operation::filter(getElements(), Criteria::isAvailable);
    }

    inline Elements getComputers() const {
        return Operation::filter(getElements(), Criteria::isComputer);
    }

    inline Elements getStores() const {
        return Operation::filter(getElements(), Criteria::isStore);
    }

    inline Elements getSwitches() const {
        return Operation::filter(getElements(), Criteria::isSwitch);
    }

    inline Elements getLinks() const {
        return Operation::filter(getElements(), Criteria::isLink);
    }
    
    //
    void setMigrationTime(const unsigned & m) {
	    migrationTime = m;
    }
   
    unsigned getMigrationTime() {
	    return migrationTime;
    }
    //
private:
	unsigned migrationTime;
};




