#pragma once

#include "defs.h"
#include "element.h"
#include "network.h"
#include "leafnode.h"
#include "edge.h"
#include "link.h"
#include "transmission.h"

#include <stdio.h>

class DijkstraAlgorithm {
	DijkstraAlgorithm() {}
	~DijkstraAlgorithm() {}
public:
	typedef std::map<Element *, std::vector<Element *> > LinksToNodes;
	//LinksToNode[Node] = vector of physical links from start_node ( candidate ) to current node
public:
	static bool findPath( Network * network, Transmission * candidate, const uint & totalWeight, const uint & observeTime);
	static void initializePathes( Network * network, LinksToNodes & pathes, Element * start);
	static Element * findNearestNode( Elements & unvisitedNodes, LinksToNodes & pathes, Network *network );
	static uint pathCost(std::vector<Element *> & path, Network * network);
	static bool checkPath(std::vector<Element*> & path, Network * network);
	static void cleanPathes( LinksToNodes & pathes, Network * network );
	static bool setMigrationLinks(std::vector<Element*> & path, Transmission * candidate, const uint & totalWeight, const uint & observeTime);
	static uint fullThroughput(const Elements & links);
};


bool DijkstraAlgorithm::findPath( Network * network, Transmission * candidate, const uint & totalWeight, const uint & observeTime) {
	Element * start = candidate->source; 
	Element * end = candidate->destination;
	Elements unvisitedNodes = network->getNodes();
	LinksToNodes pathes;
	initializePathes( network, pathes, start );//{1}
	
	while ( !unvisitedNodes.empty() ) {
		Element * currentNode = findNearestNode( unvisitedNodes, pathes, network );//{2}
		unvisitedNodes.erase( currentNode );
		Elements links = currentNode->adjacentEdges();
		
		for (Elements::iterator link=links.begin(); link!=links.end(); link++) {
			
			if ( (*link)->toLink()->getThroughput() == 0 )
				continue;
			
			for (Elements::iterator node=unvisitedNodes.begin(); node!=unvisitedNodes.end(); node++) {
				if ( !(*link)->toEdge()->connects( *node ) )
					continue;
				
				if ( pathCost( pathes[*node], network ) > ( pathCost(pathes[currentNode], network) + (*link)->toLink()->linkCost() ) ) {//{3} {4}
					
					std::vector<Element *> path = pathes[*node];
					if (path.size() == 1 && path[0]->toLink()->getThroughput()==fullThroughput(network->getLinks()))
						//если в этой вершине еще ни разу не были, то нужно удалить имеющийся там линк, который хранил "бесконечность"
						delete pathes[*node][0];

					path = pathes[currentNode];
					
					if ( path.size() == 1 && path[0]->toLink()->getThroughput()==0) {
						//если рассматриваемая вершина является непосредственным соседом стартовой,
						//то служебный линк с ( throuhgput == 0 ) не нужно помещать в pathes[*node]
						path.clear();
					}
						
					path.push_back(*link);
					pathes[*node] = path;
					break;
				}
			}
		}

	}
	
	if ( !checkPath(pathes[end], network) ) {//{5} check path from start to end
		/*нужно проверить, изменилось ли значение в векторе линков. Для этого нужно знать totalFullThroughput*/
		cleanPathes( pathes, network );//{6} 
		//удаляем те пути, которые остались не измененные,
		//т.е. содержат созданные при инициализации линки с бесконечной стоимостью
		return false;
	}
	
	cleanPathes( pathes, network );//удаляем все линки с "бесконечностью", которые использовались в процессе работы алгоритма
	
	//Если путь найден, то мы заполняем поле migrationLinks у candidate.
	//Ищем в пути самый тонкий канал  L с пропускной способностью th.
	//Также создаем tunnel с пропускной способностью throughput = th* ( candidate->weight() / totalWeight )
	//Установливаем время начала миграции --> start_time равным observeTime
	if ( setMigrationLinks(pathes[end], candidate, totalWeight, observeTime) == false ) {//{7} 
		return false;
	}
	
	//printf("hello3\n");
	return true;
}


void DijkstraAlgorithm::initializePathes( Network * network, LinksToNodes & pathes, Element * start) {
	Elements nodes = network->getNodes();
	nodes.erase(start);
	Elements links = network->getLinks();
	uint totalFullThroughput = fullThroughput(links);//{8}
	
	for (Elements::iterator node=nodes.begin(); node!=nodes.end(); node++) {
		Link * link = new Link();
		link->setThroughput(totalFullThroughput);
		std::vector<Element *> path;
		path.push_back(link);
		pathes[*node] = path;
	}
	
	Link * l = new Link();//throughput == 0
	pathes[start].push_back( l );
}

Element * DijkstraAlgorithm::findNearestNode( Elements & unvisitedNodes, LinksToNodes & pathes, Network * network ) {
	if (unvisitedNodes.empty())
		return 0;
	Element * result = (*unvisitedNodes.begin());
	uint resultCost = pathCost(pathes[result], network);
	for (Elements::iterator node=unvisitedNodes.begin(); node!=unvisitedNodes.end(); node++) {
		if ( pathes[(*node)][0]->toLink()->getThroughput() == fullThroughput(network->getLinks()) )
			//элементы с "бесконечной" стоимостью не могут быть ближайшими
			continue;
		if ( pathes[(*node)][0]->toLink()->getThroughput() == 0 )
			//элемент с нулевой стоимостью - это начальный элемент, т.е. start
			return *node;
		
		uint nodeCost = pathCost(pathes[*node], network);
		if ( nodeCost < resultCost ) {
			result = *node;
			resultCost = nodeCost;
		}
	}
	
	return result;
}


uint DijkstraAlgorithm::pathCost(std::vector<Element *> & path, Network * network) {
	
	if ( path.size() == 1 && path[0]->toLink()->getThroughput()==0 )
		return 0;
	if (path.size() == 1 && path[0]->toLink()->getThroughput()==fullThroughput(network->getLinks()) )
		return fullThroughput(network->getLinks());
	
	uint result=0;
	for ( uint i=0; i<path.size(); i++) {
		result+=path[i]->toLink()->linkCost();
	}
	return result;
}


bool DijkstraAlgorithm::checkPath(std::vector<Element*> & path, Network * network) {
	Elements links = network->getLinks();
	uint totalFullThroughput = fullThroughput(links);
	if (path.size() == 1 && path[0]->toLink()->getThroughput()==totalFullThroughput)
		return false;
	return true;
}

void DijkstraAlgorithm::cleanPathes( LinksToNodes & pathes, Network * network ) {
	uint totalFullThroughput = fullThroughput(network->getLinks());
	for (LinksToNodes::iterator i=pathes.begin(); i!=pathes.end(); i++) {
		if (i->second.size() == 1 && i->second[0]->toLink()->getThroughput()==totalFullThroughput)
			delete i->second[0];
	}
}


bool DijkstraAlgorithm::setMigrationLinks(std::vector<Element*> & path, Transmission * candidate, const uint & totalWeight, const uint & observeTime) {
	candidate->migrationLinks = OrderedElements(path.begin(), path.end());
	
	candidate->start_time = observeTime;
	Element * thinnestChannel = path[0];
	uint capacity = path[0]->toLink()->getThroughput();
	for (uint i = 1; i < path.size(); i++)
		if ( path[i]->toLink()->getThroughput() < capacity) {
			capacity = path[i]->toLink()->getThroughput();
			thinnestChannel = path[i];
		}
	
	candidate->tunnel = new Link();
	
	
	uint tunnelThroughput = capacity * ( (float)candidate->weight() / totalWeight );
	
	if ( tunnelThroughput == 0 ) {
		candidate->migrationLinks=std::vector<Element * >();
		candidate->start_time = 0;
		candidate->end_time = 0;
		delete candidate->tunnel;
		candidate->tunnel = 0;
		return false;
	}
	
	candidate->tunnel->toLink()->setThroughput(tunnelThroughput);
	return true;
}


uint DijkstraAlgorithm::fullThroughput(const Elements & links) {
	uint result = 0;
	for(Elements::iterator link=links.begin(); link!=links.end(); link++) {
		result+=(*link)->toLink()->getFullThroughput();
	}
	return result;
}


