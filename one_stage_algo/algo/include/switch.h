#pragma once

#include "node.h"
#include "link.h"
#include "criteria.h"

class Switch : public Node {
    friend class ElementFactory;
    friend class Factory;
public:
    enum Attributes {
		NONE = 0,
		ROUTER = 1,
		FW = ROUTER << 1,
		DHCP = FW << 1,
		NAT = DHCP << 1,
		PAT = NAT << 1
	};

    Switch(bool isVirtualRouter = false) : Node() {
        type = SWITCH;
        if ( isVirtualRouter )
        	attributes |= ROUTER;
    }

    virtual bool isRouter() const {
        return attributes & ROUTER; 
    }
private:

    virtual bool typeCheck(const Element * other) const {
        return Criteria::isNetwork(other);
    }

    virtual bool physicalCheck(const Element * other) const {
        return true; 
    }

private:
    // TODO: switch doesn't have any parameters, maybe latency calculation should be added on switch (buffer latency)
    //unsigned throughput;
};
