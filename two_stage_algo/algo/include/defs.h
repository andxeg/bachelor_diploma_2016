#pragma once

#include <set>
#include <map>
#include <vector>
#include <string>

// Elements
class Element;
typedef std::set<Element *> Elements;

// Physical Elements
class Node;

class Computer;
class Store;
class Switch;

class Edge;

class Link;
class Port;
typedef std::set<Port*> Ports;

class Graph;
class Network;
class Request;
class Parameter;
class ParameterValue;
typedef std::map<Parameter *, ParameterValue *> Parameters;
typedef std::set<Request *> Requests;
typedef Computer Vnf;

class Path;

//
class Transmission;
typedef std::vector < Transmission * > Transmissions;
typedef std::map <Element *, std::string > Resources;
typedef std::map <Element *, std::vector< std::string > > TenantsElements;
typedef std::vector<Element *> OrderedElements;
//
