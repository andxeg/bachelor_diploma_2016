#pragma once

#include "defs.h"
#include <string>

class Port {
public:
    Port(std::string name, Element* parentNode):
        name(name), parentNode(parentNode), assignedTo(0), assosiatedLink(0), connectedPort(0) {
        }

    inline void connect(Edge* link, Port* otherPort) {
        connectedPort =  otherPort;
        assosiatedLink = link;
    }

    inline Element* getParentNode() const {
        return parentNode;
    }

    inline std::string getName() const {
        return name;
    }

    inline Port* getConnectedPort() const {
        return connectedPort;
    }

    inline Edge* getConnectedLink() {
        return assosiatedLink;
    }

    inline void assignTo(Port* other) {
        assignedTo = other;
    }

    inline void removeAssignment() {
        assignedTo = 0;
    }

    inline Port* getAssignee() const {
        return assignedTo;
    }

private:
    std::string name;
    Port* connectedPort;
    Edge* assosiatedLink;
    Element* parentNode;

    Port* assignedTo;
};
