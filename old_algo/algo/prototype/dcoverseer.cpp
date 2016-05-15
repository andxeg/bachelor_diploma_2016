#include "dcoverseer.h"

#include "network.h"
#include "element.h"
#include "leafnode.h"
#include "operation.h"
#include "criteria.h"

#include <stdio.h>

DCOverseer::DCOverseer(const Elements & nodes) {
    Elements affineNodes = Operation::filter(nodes, Criteria::isDCLayered);
    for(Elements::iterator n = affineNodes.begin(); n != affineNodes.end(); n++) {
        LeafNode * l = (LeafNode *)(*n);
        dcs[l->dl()].insert(l);
    }
}

Elements DCOverseer::dcPool(int dc) const {
    std::map<int, Elements>::const_iterator i = dcs.find(dc);
    if ( i == dcs.end() )
        return Elements();

    return i->second;
}
 
Elements DCOverseer::dcPositionPool(int i) const {
    if ( i < 0 || i >= dcCount() )
       return Elements();

    for(std::map<int, Elements>::const_iterator dc = dcs.begin(); dc != dcs.end(); dc++ ) {
        if ( i == 0 )
            return dc->second;
        i--;
    } 
}

int DCOverseer::dcPoolId(int position) const {
    if ( position < 0 || position >= dcCount() )
       return -1;

    for(std::map<int, Elements>::const_iterator dc = dcs.begin(); dc != dcs.end(); dc++ ) {
        if ( position == 0 )
            return dc->first;
        position--;
    } 
}
