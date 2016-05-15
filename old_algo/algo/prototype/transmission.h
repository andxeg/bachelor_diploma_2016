#pragma once

#include "defs.h"
#include "element.h"
#include "leafnode.h"
#include "parameter.h"

#include <vector>
#include <stdio.h>

class Transmission {
public:
	Transmission( Element * t, Element * s, Element * d) :
		target (t),
		source (s),
		destination (d)
	{
		start_time = 0;
		end_time = 0;
		migrationLinks = std::vector<Element *>();
		tunnel = 0;
	}
	
	~Transmission() {
		if ( tunnel!= 0 )
			delete tunnel;
	}
	
	bool isValid() {
		if ( source == 0 || destination == 0 )
			return false;
		return source != destination;
	}
	
	uint weight() {
		uint result = 0;
		Parameters params = target->parameters;
		if ( target->isComputer() || target->isStore() ) {
			for (Parameters::iterator it = params.begin(); it != params.end(); it++) {
				ParameterValue * value = it->second;
				Parameter * par = it->first;
				result += value->weight();
			}
			
			if ( ( (LeafNode*)target )->getActivity() != 0 )
				result *= ( (LeafNode*)target )->getActivity();
		}
		return result;
	}
	
	bool isPassive() {
		if ( start_time == 0 && end_time == 0 ) return true;
		if ( migrationLinks.empty() ) return true;
		return false;
	}
	
	uint getActivity() {
		return ( (LeafNode*)target )->getActivity();
	}
	
	
public:
	Element * target;
	Element * source;
	Element * destination;
	uint start_time;
	uint end_time;
	OrderedElements migrationLinks;//vector of physical links
	Element * tunnel;
};