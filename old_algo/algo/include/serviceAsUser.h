#pragma once

/*
 * Service as user is actually a virtual router,
 * which is used to gain some service from external service
 *
 */
#include "computer.h"

class ServiceAsUser : public Switch {
    friend class ElementFactory;
public:
    enum Attributes {
        NONE = 0
    };

    ServiceAsUser() : Switch(true) {
    }

	virtual bool isServiceAsUser() const {
		return true;
	}

private:

    /*
     * Tenants to use this service.
     */
    ServiceAsProvider* provider;

    /*
     * Local ports used to connect to service provider
     * TODO: 1) maybe only one port should be used for one ServiceAsUser
     * TODO: 2) Virtual link should be created during tenant generation between these ports and service provider ports
     */
    Ports ImportedPorts;
};
