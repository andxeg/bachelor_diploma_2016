#pragma once

#include "node.h"

class LeafNode : public Node {
protected:
    LeafNode() : Node() {
        dcLayer = 0;
        serverLayer = 0;
	activity = 0;
	migration = 1;
    }
public:
    static int maxLayer() { return sizeof(int) * 8; }

    void setDCLayer(int l) {
        if ( l <= 0 || l > maxLayer() )
            return;

        dcLayer = 1 << (l - 1);
    }

    void setServerLayer(int l) {
        if ( l <= 0 || l > maxLayer() )
            return;

        serverLayer = 1 << (l - 1);
    }

    inline int dl() const { return dcLayer; }
    
    inline int sl() const { 
        if ( isPhysical() )
            return 0;
        return serverLayer; 
    }
    
    //
    void setActivity ( const unsigned & a ) {
	activity = a;
    }
    
    void setMigration ( const unsigned & m ) {
	    if ( m == 0 || m == 1 )
		    migration = m;
    }
    
    unsigned getActivity() { return activity; }
    unsigned getMigration() { return migration; }
    //
    
private:
    int dcLayer;
    int serverLayer;
    
    //
    unsigned activity;
    unsigned migration;
    //
};
