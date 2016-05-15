#pragma once

#include "graph.h"
#include "element.h"
#include "operation.h"
#include "criteria.h"

class Request : public Graph {
    friend class TenantXMLFactory;
public:
    Request(const Elements & e, std::string name = ""): name(name) {
        affine = false;
        provider = false;
        elements = Operation::filter(e, Criteria::isVirtual);
    }

    Request(const Request & other) { 
        affine = other.affine;
        name = other.name; 
        elements = other.elements;
    }

    inline Elements assignedElements() const {
        return Operation::filter(getElements(), Criteria::isAssigned);
    }

    inline Elements elementsToAssign() const {
        Elements assigned = assignedElements();
        return Operation::minus(getElements(), assigned);
    }

    inline Elements getMachines() const {
        return Operation::filter(getElements(), Criteria::isComputer);
    }

    inline Elements getStorages() const {
        return Operation::filter(getElements(), Criteria::isStore);
    }

    inline Elements getVSwitches() const {
        return Operation::filter(getElements(), Criteria::isSwitch);
    } 

    inline Elements getTunnels() const {
        return Operation::filter(getElements(), Criteria::isLink);
    }

    inline bool isAssigned() const {
        return elementsToAssign().empty(); 
    }

    inline void purgeAssignments() {
        Elements assigned = assignedElements();
        Operation::forEach(assigned, Operation::unassign);
    }

    inline std::string getName() const {
    	return name;
    }

    inline void addExternalLink(Element * link) {
        elements.insert(link); 
    }
    
    inline void omitElement(Element * element) {
        elements.erase(element);  
    }

    inline bool isDCAffined() const {
        return affine;
    }

    inline bool isProvider() {
        return provider;
    }

private:
    bool affine; 
    bool provider;
    std::string name;
};
