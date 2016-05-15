#pragma once

#include "defs.h"
#include "network.h"
#include "element.h"
#include "port.h"
#include "edge.h"
#include "link.h"
#include "transmission.h"
#include "dijkstra.h"


#include <vector>
#include <string>
#include <deque>
#include <algorithm>


class Migration {
public:
	Migration(Network * n, const Transmissions & t, const Resources & res, TenantsElements & tens);
	
	~Migration();
	
	void print();
	void printMigrationLinks(Transmission * task);
	bool isValid();
	bool createMigrationPlan();
	std::deque<Transmission *> sortTransmissions(Transmissions & transmissions);
	static bool compareTransmissions(Transmission * first, Transmission * second);
	uint calculateTotalTransmissionsWeight(Transmissions &  transmissions);
	
	//Methods for creating migration plan
	Transmissions findCurrentExecute(Transmissions & transmissions, const uint & observeTime);
	void createGraphRemainderResources(Transmissions & currentExecute);
	void freeGraphRemainderResources(Transmissions & currentExecute);
	uint findNearestTransmissionEnd(Transmissions & currentExecute, const uint & observeTime);
	bool checkTime( Transmission * candidate, const uint & migrationTime);
	void clearCandidate( Transmission * candidate );
	Transmissions findIntersectingTasks(Transmissions & transmissions, const uint & start, const uint & end);
	static bool compareIntersectingTasks(Transmission * first, Transmission * second);
	bool checkMigration(Network * network, Transmission * candidate);
	uint updateCurrentTime(Transmissions & transmissions);
	
private:
	Resources resources;
	TenantsElements tenantsElements;
	Network * network;
	Transmissions tasks;
	uint spentMigrationTime;
};


//======================================METHODS================================

Migration::Migration(Network * n, const Transmissions & t, const Resources & res, TenantsElements & tens) :
	network(n),
	resources(res),
	tenantsElements(tens),
	tasks (t),
	spentMigrationTime(0)
{}

Migration::~Migration() {
	for ( uint i = 0; i < tasks.size(); i++) {
		delete tasks[i];
	}
}

//after creating migration plan this method must print all information about Transmissions
void Migration::print() {
	printf("[MIGRATION]");
	printf("\nThe time allocated for the migration ->%u\n ", network->getMigrationTime());
	
	if ( spentMigrationTime != 0 )
		printf("The time spent for the migration ->%u\n ", spentMigrationTime);
	
	for ( uint i = 0; i < tasks.size(); i++) {
		if ( tasks[i]->isValid() )
			printf("\t[Transmission #%d]: Tenant -> %s; Element -> %s; From -> %s; To -> %s\n",
				i, tenantsElements[tasks[i]->target][0].c_str(), 
				tenantsElements[tasks[i]->target][1].c_str(),
				resources[tasks[i]->source].c_str(),
				resources[tasks[i]->destination].c_str() );
			if ( tasks[i]->tunnel != 0 ) {
				printf("\t\tThroughput of migration channel ->%u\n", tasks[i]->tunnel->toLink()->getThroughput());
				printf("\t\tStart ->%u End -> %u\n", tasks[i]->start_time, tasks[i]->end_time);
				printf("\t\t[PATH]\n");
				//здесь нужно вывести путь миграции для конкретного перемещения с указанием узлов и портов
				printMigrationLinks(tasks[i]);
			}

	}
	
}

void Migration::printMigrationLinks(Transmission * task) {

	OrderedElements path = task->migrationLinks;
	for (uint i = 0; i < path.size(); i++) {
		Element * e = path[i];
		Port * port1 =  e->toEdge()->getFirst();
		Port * port2 = e->toEdge()->getSecond();
		printf("\t\t\t%s:%s--%s:%s;",
			resources[port1->getParentNode()].c_str(), port1->getName().c_str(),
			resources[port2->getParentNode()].c_str(), port2->getName().c_str() );
		printf("\n");
	}
	printf("\n");
}

//class Migration keeps only Transmissions with source != destination
bool Migration::isValid() {
	if ( tasks.empty() ) return false;
	for ( uint i = 0; i < tasks.size(); i++) {
		if ( tasks[i]->isValid() )
			return true;
	}
	return false;
}


bool Migration::createMigrationPlan() {
	uint migrationTime = network->getMigrationTime();
	uint totalWeight = calculateTotalTransmissionsWeight(tasks);
	std::deque<Transmission *> queue = sortTransmissions(tasks);//{0}
	std::deque<Transmission *> q1( queue );//This queue need to check for infinite loops
	uint l = queue.size();//looping when l == 0 and q1 == current queue
	uint currentTime = 0;
	bool success;
	
	while ( l!=0 || q1!=queue ) {
		
		uint observeTime = 0;
		success = false;
		//если бы очередь была пустой, то это означало бы, что миграция закончилась на предыдущей итерации
		if ( queue.empty() ) { 
			spentMigrationTime=currentTime;
			return true;
		}
		
		Transmission * candidate = queue.front();
		queue.pop_front();
		
		while ( observeTime!=currentTime ) {
			Transmissions currentExecute = findCurrentExecute(tasks, observeTime);//{1}
			createGraphRemainderResources(currentExecute);//{2} 

			//Проверяем, есть ли место на destination для виртуального элемента
			//Если нет, то пропускаем самую ближайшую работу
			if ( candidate->destination->canHostAssignment(candidate->target) == false ) {
				freeGraphRemainderResources(currentExecute);//{4}
				observeTime = findNearestTransmissionEnd(currentExecute, observeTime);//{5}
				continue;
			}
			//

			if ( DijkstraAlgorithm::findPath(network, candidate, totalWeight, observeTime) == false ) {//{3}
				freeGraphRemainderResources(currentExecute);//{4}
				//так как нам скорее всего мешает одна работа из currentExecute,
				//то мы пропускаем самую ближайшую
				observeTime = findNearestTransmissionEnd(currentExecute, observeTime);//{5} 
				continue;
			} 
			
			
			//====================================================================================
			//если путь нашелся в графе, то нужно сделать еще несколько проверок
			
			if ( checkTime( candidate, migrationTime) == false ) {//{6}
				clearCandidate( candidate );//{7} удаляем информацию о текущем поиске
				freeGraphRemainderResources(currentExecute);
				observeTime = findNearestTransmissionEnd(currentExecute, observeTime);
				continue;
			}
			
			//====================================================================================
			//если же умещаемся пока в Tdir, то нужно проверить, мешаем ли мы каким-нибудь перемещениям
			success = true;
			freeGraphRemainderResources(currentExecute);
			Transmissions intersectingTasks = findIntersectingTasks(tasks, candidate->start_time, candidate->end_time);//{8}
			//пересекающиеся перемещения упорядочены по возрастанию start_time
			//если пересекающихся с рассматриваевым перемещений нет, то считаем перемещение успешным
			if ( intersectingTasks.size() != 0 ) {
				for ( uint time = intersectingTasks[0]->start_time; time <= intersectingTasks.back()->start_time; time++) {
					//printf("time->%u\n", time);
					Transmissions currExec=findCurrentExecute(intersectingTasks, time);
					createGraphRemainderResources(currExec);
					//Проверяем, есть ли место на destination для виртуального элемента
					//Также проверяем доступную пропускную способность канала
					if ( checkMigration(network, candidate) == false ) {//{9}
						clearCandidate( candidate );
						freeGraphRemainderResources(currExec);
						observeTime = intersectingTasks[0]->end_time;
						success = false;
						break;
					}
					freeGraphRemainderResources(currExec);
				}
			}
			
			if ( success == false )
				continue;
			
			//если удалось назначить работу, т.е. success == true
			q1 = queue;
			l = queue.size();
			currentTime = updateCurrentTime(tasks);//{10} текущее время - это конец самого позднего перемещения
			candidate->target->unassign();
			candidate->destination->assign(candidate->target);
			break;
		}
		
		if ( success == true )
			continue;
		//if success == false => observeTime == currentTime
		//если попали сюда, то поставить работу можно только в конец расписания
		//в конце расписания никаких перемещений нет;
		//по предположению, если нет никаких перемещений, следовательно, все 
		//сетевые ресурсы свободны => следить нужно только за Tdir
		// в данной точке программы network - это граф до начала миграции
		//!!!
		//Но для начала нужно проверить, есть ли место для виртуального элемента на destination
		if ( candidate->destination->canHostAssignment( candidate->target ) == false ) {
			queue.push_back( candidate );
			l = l - 1;
			continue;
		}
		//

		if ( DijkstraAlgorithm::findPath(network, candidate, totalWeight, observeTime) == false ) {
			queue.push_back( candidate );
			l = l - 1;
			continue;
		}
		
		//если путь найден, то проверям, укладываемся мы в Tdir или нет
		if ( checkTime( candidate, migrationTime) == false ) {
			//план миграции не может быть построен
			//candidate  может разместиться только в конце расписания
			printf("Migration time is exceeded\n");
			//Но во время миграции не укладываемся
			//даже при обладании всех сетевых ресурсов
			return false;
		}


		//если умещаемся в Tdir, то обновляем очередь и текущее время, а также меняем назначение candidate на новое
		q1 = queue;
		l = queue.size();
		currentTime = updateCurrentTime(tasks);
		candidate->target->unassign();
		candidate->destination->assign(candidate->target);
		//можно прибавить candidate->duration к текущему времени, чтобы получить новое currentTime,
		//так как он все равно встал в конец расписания
		continue;
		
	}//end main while
	
	if ( queue.empty() ) { 
		spentMigrationTime=currentTime;
		return true;
	}
	
	printf("Infinite loops\n");
	return false;
}


std::deque<Transmission *> Migration::sortTransmissions(Transmissions &  transmissions) {
	std::sort(transmissions.begin(), transmissions.end(), compareTransmissions);
	std::deque<Transmission *> result(transmissions.begin(),transmissions.end());
	return result;
}

bool Migration::compareTransmissions(Transmission * first, Transmission * second) {
	return first->weight() > second->weight();
}

uint Migration::calculateTotalTransmissionsWeight(Transmissions &  transmissions) {
	uint result = 0;
	for ( uint i = 0; i < transmissions.size(); i++) {
		result += transmissions[i]->weight();
	}
	return result;
}

Transmissions Migration::findCurrentExecute(Transmissions & transmissions, const uint & observeTime) {
	Transmissions result;
	for ( uint i = 0; i < transmissions.size(); i++) {
		if ( transmissions[i]->isPassive() ) continue;
		if ( ( observeTime >= transmissions[i]->start_time ) && ( observeTime < transmissions[i]->end_time ) )
			result.push_back( transmissions[i] );
	}
	return result;
}

void Migration::createGraphRemainderResources(Transmissions & currentExecute) {
	for ( uint i = 0; i < currentExecute.size(); i++ ) {
		OrderedElements links = currentExecute[i]->migrationLinks;
		Element * tun = currentExecute[i]->tunnel;
		for ( OrderedElements::iterator j = links.begin(); j != links.end(); j++ )
			(*j)->toLink()->assignLink(tun);
		//Для мигрирующих элементов виртуальный элемент должен находиться и на source, и на destination
		//Метод assign не назначает заново виртуальный элемент, если он был назначен ранее
		//Метод unassign тоже отрабатывает коррректно, даже если виртуальный элемент никуда не был назначен
		Element * destination = currentExecute[i]->destination;
		Element * source = currentExecute[i]->source;
		Element * target = currentExecute[i]->target;
		if ( destination->assign( target ) == false || source->assign( target ) == false )
			printf("Error while assign in Migration::CreateGraphRemainderResources");
		//
	}
}

void Migration::freeGraphRemainderResources(Transmissions & currentExecute) {
	for ( uint i = 0; i < currentExecute.size(); i++) {
		OrderedElements links = currentExecute[i]->migrationLinks;
		Element * tun = currentExecute[i]->tunnel;
		for ( OrderedElements::iterator j = links.begin(); j != links.end(); j++ )
			(*j)->toLink()->unassignLink(tun);
		//Если у виртуального элемента migrationLinks != 0 ,следовательно, его оставляем на destination,
		//иначе - на source, так как он еще не смигрировал
		Element * destination = currentExecute[i]->destination;
		Element * source = currentExecute[i]->source;
		Element * target = currentExecute[i]->target;
		OrderedElements migrationLinks = currentExecute[i]->migrationLinks;
		if ( migrationLinks.size() == 0 ) {
			target->unassign();
			if ( source -> assign( target ) == false )
				printf("Error while assign in Migration::freeGraphRemainderResources");
		} else {
			target->unassign();
			if ( destination -> assign( target ) == false )
				printf("Error while assign in Migration::freeGraphRemainderResources");
		}
		//
	}
}

uint Migration::findNearestTransmissionEnd(Transmissions & currentExecute, const uint & observeTime) {
	uint result;
	if ( currentExecute.empty()) {
		printf("Empty currentExecute in method Migration::findNearestTransmissionEnd\n");
		return (observeTime + 1);
	}
	
	result = network->getMigrationTime();
	for ( uint i = 0; i < currentExecute.size(); i++) {
		if ( currentExecute[i]->end_time > observeTime ) {
			uint t = currentExecute[i]->end_time - observeTime;
			if ( t < (result - observeTime) )
				result = currentExecute[i]->end_time;
		}
	}
	return result;
}

bool Migration::checkTime( Transmission * candidate, const uint & migrationTime) {
	uint capacity = candidate->tunnel->toLink()->getThroughput();
	uint activity = candidate->getActivity();
	
	if ( activity >= capacity )//в этом случае миграция не завершима
		return false;
	
	uint moveInformation;
	if ( activity != 0 )
		moveInformation = candidate->weight() / activity;//объем V, например размер storage-элемента
	else 
		moveInformation = candidate->weight();
	
	uint duration = moveInformation / ( capacity - activity );
	
	if ( ( candidate->start_time + duration ) > migrationTime )
		return false;
	
	candidate->end_time = candidate->start_time + duration;
	return true;
}

void Migration::clearCandidate( Transmission * candidate ) {
	candidate->start_time = 0;
	candidate->end_time = 0;
	candidate->migrationLinks=std::vector<Element * >();
	delete candidate->tunnel;
	candidate->tunnel = 0;
}

Transmissions Migration::findIntersectingTasks(Transmissions & transmissions, const uint & start, const uint & end) {
	Transmissions result;
	for ( uint i = 0; i < transmissions.size(); i++) {
		if ( transmissions[i]->isPassive() ) continue;
		if ( ( start < transmissions[i]->start_time ) && ( transmissions[i]->start_time < end ) )
			result.push_back(transmissions[i]);
	}
	
	std::sort(result.begin(), result.end(), compareIntersectingTasks);
	return result;
}


bool Migration::compareIntersectingTasks(Transmission * first, Transmission * second) {
	return first->start_time < second->start_time;
}

bool Migration::checkMigration(Network * network, Transmission * candidate) {
	OrderedElements links = candidate->migrationLinks;
	Element * tun = candidate->tunnel;

	if ( candidate->destination->canHostAssignment(candidate->target) == false )
		return false;

	for ( OrderedElements::iterator j = links.begin(); j != links.end(); j++ )
		if ( (*j)->toLink()->canHostAssignment(tun) == false )
			return false;
	
	return true;
}

uint Migration::updateCurrentTime(Transmissions & transmissions) {
	uint result = 0;
	for ( uint i = 0; i < transmissions.size(); i++) {
		if ( transmissions[i]->isPassive() ) continue;
		if ( result < transmissions[i]->end_time ) 
			result = transmissions[i]->end_time;
	}
	return result;
}
