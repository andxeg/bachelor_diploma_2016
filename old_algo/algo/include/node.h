#pragma once

#include "element.h"
#include "criteria.h"
#include "edge.h"
#include "port.h"
#include "link.h"
#include "operation.h"

class Node : public Element {
	friend class TenantXMLFactory;
protected:
    Node() : Element() {}
public:
    Elements & getEdges() {
        return edges;
    }

    virtual ~Node() {
            for(Ports::iterator i = ports.begin(); i != ports.end(); i++)
                delete *i;
    }

    bool addEdge(Element * element) {
        if ( Criteria::isEdge(element) )
            return false;
        edges.insert(element);
        return true;
    }

    bool hasEdge(Element * element) {
        if ( !Criteria::isEdge(element) ) return false;
        return edges.find(element) != edges.end(); 
    }

    bool addPort(Port * port) {
    	ports.insert(port);
		return true;
    }

    const Ports& getPorts() const {
        return ports;
    }

    void removePort(Port * port) {
        ports.erase(port);
    }

    Port* getPortByName(std::string name) const {
        for (Ports::const_iterator it = ports.begin(); it != ports.end(); ++it )
            if ( (*it)->getName().compare(name) == 0 )
                return *it;
        return 0;
    }

    Port* getFreePort() {
        for ( Ports::iterator it = ports.begin(); it != ports.end(); ++it ) {
            if ( (*it)->getConnectedLink() == 0 ) {
                return *it;
            }
        }
        return 0;
    }

    virtual Elements adjacent() const {
        return edges;
    }

    virtual Elements adjacentNodes() const {
        Elements result;
        Elements edges = adjacentEdges();
        for (Elements::iterator i = edges.begin(); i != edges.end(); i++) {
            Edge * edge = (*i)->toEdge();
            result.insert(edge->getAdjacent(this));
        }
        return result;
    }

    virtual Elements adjacentEdges() const {
        Elements result;
        for(Ports::iterator i = ports.begin(); i != ports.end(); i++ ) {
            Port * p = *i;
            Edge * e = p->getConnectedLink();
            if ( e != 0 )
                result.insert(e);
        }
        return result;
    }

    virtual void unassign() {
        if ( !isAssigned() )
            return;

        Elements tunnels = adjacentEdges();
        for (Elements::iterator i = tunnels.begin(); i != tunnels.end(); i++) {
            Link * tunnel = (*i)->toLink();
            Operation::unassignLink(tunnel);
        }

        assignee->removeAssignment(this);
    }

private:
    Elements edges;
    Ports ports;
};
