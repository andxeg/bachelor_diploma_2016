#pragma once

/*
 * Service as provider is actually a vnf (which is actually a VM),
 * which provides some ports to other Tenants (other requests)
 *
 */
#include "computer.h"

class ServiceAsProvider : public Vnf {
    friend class ElementFactory;
public:
    enum Attributes {
        NONE = 0
    };

    ServiceAsProvider() : Computer(true) {
    }

	bool isServiceAsProvider() const {
		return true;
	}

private:

    /*
     * Tenants to use this service.
     */
    Requests serviceUsers;

    /*
     * Ports used to provide service.
     * These ports are mapped by client.
     */
    Ports externalPorts;
};
