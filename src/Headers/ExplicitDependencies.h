/*
 * Copyright 2016 Pallavi Maiya, Rahul Gupta and Aditya Kanade, Indian Institute of Science
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Contains structures which store and manage explicit HB info obtained from dependence.txt gives as input.
 */

#ifndef EXPLICITDEPENDENCIES_H_
#define EXPLICITDEPENDENCIES_H_

#include <iostream>
#include <set>
#include <map>
#include <iomanip>

#include <Debug.h>


using namespace std;

struct ExplicitDependencies{
private:
	map < int, set<int> > sourceToTargetMap;				// key - source program-index, value - vector of target program-indices
	map < int, set<int> > targetToSourceMap;				// key - target program-index, value - vector of source program-indices
public:
	void addDependency(int src, int dest);
	bool isDependent(int dest);
	bool isSource(int source);
	void removeSourceFromTargetToSourceMap(int dest, int source);
	void resolveDependency(int source);
	void dumpSourceToTargetMap();
	void dumpTargetToSourceMap();
	void dumpDependencies();
	bool isEdge(int source, int dest);
	//@warning: should be called from replica(backup) object, as we'll keep modifying working object.
	set<int> getSources(int target);
	map < int, set<int> > getSourceToTargetMap();
	map < int, set<int> > getTargetToSourceMap();
};

void ExplicitDependencies::addDependency(int src, int dest){
	map < int, set<int> > :: iterator it = sourceToTargetMap.find(src);
	set<int> v;
	if(it != sourceToTargetMap.end())
		v = it->second;
	v.insert(dest);
	sourceToTargetMap[src] = v;

	it = targetToSourceMap.find(dest);
	set<int> vv;
	if(it != targetToSourceMap.end())
		vv = it->second;
	vv.insert(src);
	targetToSourceMap[dest] = vv;
}

bool ExplicitDependencies::isDependent(int dest){
	map < int, set<int> > :: iterator it = targetToSourceMap.find(dest);
	if(it != targetToSourceMap.end())
		return true;
	return false;
}

bool ExplicitDependencies::isSource(int source){
	map < int, set<int> > :: iterator it = sourceToTargetMap.find(source);
	if(it != sourceToTargetMap.end())
		return true;
	return false;
}

void ExplicitDependencies::removeSourceFromTargetToSourceMap(int target, int source){
	map < int, set<int> > :: iterator iit = targetToSourceMap.find(target);
	if(iit != targetToSourceMap.end()){
		set<int> s = iit->second;
		s.erase(s.find(source));
		if(s.empty())
			targetToSourceMap.erase(target);
		else
			targetToSourceMap[target] = s;
	}
}

void ExplicitDependencies::resolveDependency(int source){
	map < int, set<int> > :: iterator it = sourceToTargetMap.find(source);
	if(it != sourceToTargetMap.end()){
		set<int> destSet = it->second;
		for(set<int> :: iterator vit = destSet.begin(); vit!=destSet.end(); ++vit){
			int target =  *vit;
			removeSourceFromTargetToSourceMap(target,source);
		}
		sourceToTargetMap.erase(source);
	}
}

bool ExplicitDependencies::isEdge(int source, int dest){
	map < int, set<int> > :: iterator it = sourceToTargetMap.find(source);
	if(it != sourceToTargetMap.end()){
		set<int> destSet = it->second;
		if(destSet.find(dest) != destSet.end())
			return true;
	}
	return false;
}

//should be called within Debug() 
void ExplicitDependencies::dumpSourceToTargetMap(){
	cerr<<"Source to Target Map dump: "<<endl;
	for(map < int, set<int> > :: iterator it = sourceToTargetMap.begin(); it != sourceToTargetMap.end(); ++it){
		set<int> s = it->second;
		cerr<<setw(5)<<it->first<<"  --->  ";
		for(set<int>::iterator sit = s.begin(); sit != s.end(); ++sit)
			cerr<<setw(5)<<*sit<<" ,";
		cerr<<endl;
	}
}

//should be called within Debug() 
void ExplicitDependencies::dumpTargetToSourceMap(){
	cerr<<"Target to Source Map dump: "<<endl;
	for(map < int, set<int> > :: iterator it = targetToSourceMap.begin(); it != targetToSourceMap.end(); ++it){
		set<int> s = it->second;
		cerr<<setw(5)<<it->first<<"  --->  ";
		for(set<int>::iterator sit = s.begin(); sit != s.end(); ++sit)
			cerr<<setw(5)<<*sit<<" ,";
		cerr<<endl;
	}
}

void ExplicitDependencies::dumpDependencies(){
	Debug(dumpSourceToTargetMap());
	Debug(dumpTargetToSourceMap());
}


//@warning: should be called from replica(backup) object, as we'll keep modifying working object.
set<int> ExplicitDependencies::getSources(int target){
	map < int, set<int> > :: iterator it = targetToSourceMap.find(target);
	set<int> s;
	if(it != targetToSourceMap.end())
		s = it->second;
	return s;
}

map < int, set<int> > ExplicitDependencies::getSourceToTargetMap(){
	return sourceToTargetMap;
}

map < int, set<int> > ExplicitDependencies::getTargetToSourceMap(){
	return targetToSourceMap;
}


#endif // EXPLICITDEPENDENCIES_H_
