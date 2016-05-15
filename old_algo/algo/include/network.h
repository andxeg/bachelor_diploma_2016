#pragma once

#include "graph.h"
#include "element.h"
#include "operation.h"
#include "criteria.h"
#include "link.h"
#include <stdio.h>

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

    std::map<std::string, double> getServersParameters() {
        std::map<std::string, double> result;
        Elements computers = getComputers();
        result = (*computers.begin())->getParametersTotal();
        Elements::iterator first = ++computers.begin();
        Elements::iterator back = computers.end();
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
    }


    std::map<std::string, double> getUsedServersParameters() {
        std::map<std::string, double> result;
        Elements computers = getComputers();
        result = (*computers.begin())->getParametersUsed();
        Elements::iterator first = ++computers.begin();
        Elements::iterator back = computers.end();
        for (Elements::iterator i = first; i != back; i++) {
            Element * computer = *i;
            std::map<std::string, double> params = computer->getParametersUsed();
            std::map<std::string, double>::iterator firstParam = params.begin();
            std::map<std::string, double>::iterator lastParam = params.end();
            for (std::map<std::string, double>::iterator j = firstParam; j != lastParam; j++) {
                std::string paramName = j->first;
                double paramValue = j->second;
                result[paramName] += paramValue;
            }

        }
        return result;
    }

    std::map<std::string, double> getStoragesParameters() {
        std::map<std::string, double> result;
        Elements storages = getStores();
        result = (*storages.begin())->getParametersTotal();
        Elements::iterator first = ++storages.begin();
        Elements::iterator back = storages.end();
        for (Elements::iterator i = first; i != back; i++) {
            Element * storage = *i;
            std::map<std::string, double> params = storage->getParametersTotal();
            std::map<std::string, double>::iterator firstParam = params.begin();
            std::map<std::string, double>::iterator lastParam = params.end();
            for (std::map<std::string, double>::iterator j = firstParam; j != lastParam; j++) {
                std::string paramName = j->first;
                double paramValue = j->second;
                result[paramName] += paramValue;
            }
        }
        return result;
    }

    std::map<std::string, double> getUsedStoragesParameters() {
        std::map<std::string, double> result;
        Elements storages = getStores();
        result = (*storages.begin())->getParametersTotal();
        Elements::iterator first = ++storages.begin();
        Elements::iterator back = storages.end();
        for (Elements::iterator i = first; i != back; i++) {
            Element * storage = *i;
            std::map<std::string, double> params = storage->getParametersUsed();
            std::map<std::string, double>::iterator firstParam = params.begin();
            std::map<std::string, double>::iterator lastParam = params.end();
            for (std::map<std::string, double>::iterator j = firstParam; j != lastParam; j++) {
                std::string paramName = j->first;
                double paramValue = j->second;
                result[paramName] += paramValue;
            }
        }
        return result;
    }

    double getLinksParameters() {
        double result = 0.0;
        Elements links = getLinks();
        Elements::iterator first = links.begin();
        Elements::iterator back = links.end();
        for (Elements::iterator i = first; i != back; i++) {
            //result += static_cast<double>((*i)->toLink()->getFullThroughput());
            result += static_cast<double>(((Link *)(*i))->getFullThroughput());
        }
        return result;
    }

    double getUsedLinksParameters() {
        double result = 0.0;
        Elements links = getLinks();
        Elements::iterator first = links.begin();
        Elements::iterator back = links.end();
        for (Elements::iterator i = first; i != back; i++) {
            result += static_cast<double>((*i)->toLink()->getFullThroughput() -
                    (*i)->toLink()->getThroughput());
        }
        return result;
    }
        //
private:
	unsigned migrationTime;
};




