#include "dijkstrarouter.h"

#include "network.h"
#include "link.h"

DijkstraRouter::DijkstraRouter(Link * tun, Element * f, Element * t, Network * n)
:
    Router(n),
    from(f),
    to(t),
    tunnel(tun)
{
}

bool DijkstraRouter::route() {
    return true;
}
