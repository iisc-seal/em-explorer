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

#ifndef EXPLICITDEPENDENCIES_H_
#define EXPLICITDEPENDENCIES_H_

#include <iostream>
#include <set>
#include <map>
#include <iomanip>

using namespace std;

struct ExplicitDependencies{
private:
	map < int, set<int> > sourceToTargetMap;				// key - source program-index, value - vector of target program-indices
	//map < int, set<int> > targetToSourceMap;				// key - target program-index, value - vector of source program-indices
public:
	void addDependency(int src, int dest);
	void dumpSourceToTargetMap();
	map < int, set<int> > getSourceToTargetMap();
};

void ExplicitDependencies::addDependency(int src, int dest){
	map < int, set<int> > :: iterator it = sourceToTargetMap.find(src);
	set<int> v;
	if(it != sourceToTargetMap.end())
		v = it->second;
	v.insert(dest);
	sourceToTargetMap[src] = v;

	/*it = targetToSourceMap.find(dest);
	set<int> vv;
	if(it != targetToSourceMap.end())
		vv = it->second;
	vv.insert(src);
	targetToSourceMap[dest] = vv;*/
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


map < int, set<int> > ExplicitDependencies::getSourceToTargetMap(){
	return sourceToTargetMap;
}




#endif // EXPLICITDEPENDENCIES_H_
