#include "dijkstrarouter.h"
#include "network.h"

#define INF 1000000000


bool comparator(std::pair<uint, Element *> first , std::pair<uint, Element *> second ) {
    return first.first < second.first;
}


DijkstraRouter::DijkstraRouter(Element * start, Network * n) :
        start(start), network(n) {}

void DijkstraRouter::route() {
    //find pathes from start element to all element in graph network
    Elements nodes = network->getNodes();
    ulong nodesNum = nodes.size();
    std::map<Element *, uint > dist;
    std::map<Element *, Element *> ancestors;
    for (Elements::iterator i = nodes.begin(); i != nodes.end(); i ++) {
        dist[*i] = INF;
    }
    dist[start] = 0;
    std::set < std::pair<uint, Element *>,
            bool (*) (std::pair<uint, Element *>, std::pair<uint, Element *>)> queue(comparator);

    //std::set < std::pair<uint, Element *> > queue;

    queue.insert (std::make_pair(dist[start], start));

    while (!queue.empty()) {
        Element * node = queue.begin()->second;
        queue.erase(queue.begin());

        Elements adjacentElements = node->adjacentNodes();
        for (Elements::iterator i = adjacentElements.begin(); i != adjacentElements.end(); i++) {
            Element * to = (*i);
            uint len = 1;// in common case : len = distance from node to (*i)
            if (dist[node] + len < dist[to]) {
                queue.erase(std::make_pair(dist[to], to));
                dist[to] = dist[node] + len;
                ancestors[to] = node;
                queue.insert(std::make_pair(dist[to], to));
            }

        }

    }

    for (Elements::iterator j = nodes.begin(); j != nodes.end(); j++) {
        Element * elem = (*j);
        std::vector<Element *> path;
        Element * k = elem;
        Element * ancestor;
        Element * link;
        if (ancestors.count(k) == 0 || k == start)
            continue;

        while (k != start) {
            ancestor = ancestors[k];
            Elements edges = ancestor->adjacentEdges();
            for (Elements::iterator e = edges.begin(); e != edges.end(); e++) {
                if ((*e)->toEdge()->connects(k)) {
                    link = (*e);
                    break;
                }

            }

            path.push_back(link);
            k = ancestor;
        }

        std::reverse(path.begin(), path.end());
        pathes[elem] = path;
    }

}


std::map<Element *, std::vector <Element * > > DijkstraRouter::getPathes() {
    return pathes;
};
