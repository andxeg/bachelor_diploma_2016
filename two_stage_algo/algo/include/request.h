#pragma once

#include "graph.h"
#include "element.h"
#include "operation.h"
#include "criteria.h"
#include "link.h"
#include <iostream>

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

    inline bool isAssignedWithoutChannel() const {
        for (Elements::iterator i = elements.begin(); i != elements.end(); i++) {
            Element * element = *i;
            if (element->isLink())
                continue;

            if (!element->isAssigned())
                return false;
        }

        return true;
    }

    inline void purgeAssignments() {
        std::cout << "IN purgeAssignments" << std::endl;
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


    //
    void printParameters(std::map<std::string, double> params) {
        std::map<std::string, double>::iterator i = params.begin();
        std::map<std::string, double>::iterator last = params.end();
        for (; i != last; i++) {
            std::cout << "Name -> "  << i->first << " Value -> " << i->second << std::endl;
        }
        std::cout << std::endl << std::endl;
    }

    std::map<std::string, double> getAllVMsParameters() {
        std::map<std::string, double> result;
        Elements computers = getMachines();
        if (computers.size() == 0)
            return result;
        result = (*computers.begin())->getParametersTotal();
        //std::cout << std::endl <<  *computers.begin() << std::endl;
        Elements::iterator first = ++computers.begin();
        Elements::iterator back = computers.end();
        for (Elements::iterator i = first; i != back; i++) {
            Element * computer = *i;
            std::map<std::string, double> params = computer->getParametersTotal();

            //std::cout << computer << std::endl;

            std::map<std::string, double>::iterator firstParam = params.begin();
            std::map<std::string, double>::iterator lastParam = params.end();
            for (std::map<std::string, double>::iterator j = firstParam; j != lastParam; j++) {
                std::string paramName = j->first;
                double paramValue = j->second;
                result[paramName] += paramValue;
            }
        }
        return result;
    };

    std::map<std::string, double> getAllStParameters() {
        std::map<std::string, double> result;
        Elements storages = getStorages();
        if (storages.size() == 0)
            return result;
        result = (*storages.begin())->getParametersTotal();
        Elements::iterator first = ++storages.begin();
        Elements::iterator back = storages.end();
        for (Elements::iterator i = first; i != back; i++) {
            Element * computer = *i;
            std::map<std::string, double> params = computer->getParametersTotal();
            std::map<std::string, double>::iterator firstParam = params.begin();
            std::map<std::string, double>::iterator lastParam = params.end();
            for (std::map<std::string, double>::iterator j = firstParam; j != lastParam; j++) {
                std::string paramName = j->first;
                double paramValue = j->second;
                result[paramName] += paramValue;
            }
        }
        return result;
    };

    double getAllLinksParameters() {
        double result = 0.0;
        Elements links = getTunnels();
        Elements::iterator first = links.begin();
        Elements::iterator back = links.end();
        for (Elements::iterator i = first; i != back; i++) {
            //result += static_cast<double>((*i)->toLink()->getFullThroughput());
            result += static_cast<double>(((Link *)(*i))->getFullThroughput());
        }
        return result;
    };

    double weight() {
        double result = 0.0;
        std::map<std::string, double> parameters = getAllVMsParameters();
        for (std::map<std::string, double>::iterator i = parameters.begin(); i != parameters.end(); i++) {
            result += i->second;
        }

        parameters = getAllStParameters();
        for (std::map<std::string, double>::iterator i = parameters.begin(); i != parameters.end(); i++) {
            result += i->second;
        }

        return result + getAllLinksParameters();
    }
    //

private:
    bool affine; 
    bool provider;
    std::string name;
};
