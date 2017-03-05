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

#ifndef REORDEREDPOSTS_H_
#define REORDEREDPOSTS_H_

#include <iostream>
#include <set>
#include <map>
#include <iomanip>

#include <Debug.h>

using namespace std;

// client has the responsibility to map input indices of this class from control location to program index
// and reverse map the outputs of this class.
struct ReorderedPosts{
private:
	// reordered post pairs are originally stored in this map
	map < int, set<int> > sourceToTargetMap;	// key - source program-index, value - vector of target program-indices
	// source resolved pairs are stored in this map.
	map < int, set<int> > targetToSourceMap;	// key - target program-index, value - vector of source program-indices
public:
	// if p1 (post1) comes before p2 (post2) and findTarget(sequence, p1, p2) reorders these two then p2 is considered  as source and p1 as target
	void addPair(int src, int dest);
	void addResolvedPair(int dest, int src);
	void resolveDependency(int source);
	set< pair<int,int> > processPost(int postIndex);
	void dumpSourceToTargetMap();
	void dumpTargetToSourceMap();
	void dumpDependencies();
	map < int, set<int> > getSourceToTargetMap();
	map < int, set<int> > getTargetToSourceMap();
	void erase();
};

void ReorderedPosts::addPair(int src, int dest){
	map < int, set<int> > :: iterator it = sourceToTargetMap.find(src);
	set<int> v;
	if(it != sourceToTargetMap.end())
		v = it->second;
	v.insert(dest);
	sourceToTargetMap[src] = v;
}

// overwriting src to add only nearest reordered source post.
void ReorderedPosts::addResolvedPair(int dest, int src){
	set<int> vv;
	vv.insert(src);
	/* targetToSourceMap gets populated only when a src is executed in the sequence.
	 * This map is consulted only when a target is executed so that the algo can figure
	 * out which all posts have been reordered w.r.t. the target and add post-to-post HB accordingly.
	 * Note that since we are creating a new src set each time addResolvedPair is called,
	 * the below set will be a singleton set containing the nearest reordered post w.r.t. dest.
	 * This is what is needed to add HB (refer Algorithm Section in the paper).
	 */
	targetToSourceMap[dest] = vv;
}

void ReorderedPosts::resolveDependency(int source){
	map < int, set<int> > :: iterator it = sourceToTargetMap.find(source);
	if(it != sourceToTargetMap.end()){
		set<int> destSet = it->second;
		for(set<int> :: iterator vit = destSet.begin(); vit!=destSet.end(); ++vit){
			int target =  *vit;
			addResolvedPair(target,source);
		}
		sourceToTargetMap.erase(source);
	}
}

// returns HBEdges to be added.
set< pair<int,int> > ReorderedPosts::processPost(int postIndex){
	Debug(cerr<<"begin :: ProcessPost ( "<<postIndex<<" )"<<endl);
	dumpDependencies();

	set< pair<int,int> > hbEdges;
	int source = postIndex;
	resolveDependency(source);
	Debug(cerr<<"Post resolution maps - "<<endl);
	dumpDependencies();

	int dest = postIndex;
	map < int, set<int> > :: iterator it = targetToSourceMap.find(dest);
	if(it != targetToSourceMap.end()){
		set<int> sourceSet = it->second;
		for(set<int> :: iterator vit = sourceSet.begin(); vit!=sourceSet.end(); ++vit){
			int src =  *vit;
			hbEdges.insert(make_pair(src,dest));
			Debug(cerr<<"Inserting HB Edges - "<<src<<"-->"<<dest<<endl);
		}
		targetToSourceMap.erase(dest);
	}
	return hbEdges;
}

//should be called within Debug()
void ReorderedPosts::dumpSourceToTargetMap(){
	cerr<<"unresolved reordered posts (source to target): "<<endl;
	for(map < int, set<int> > :: iterator it = sourceToTargetMap.begin(); it != sourceToTargetMap.end(); ++it){
		set<int> s = it->second;
		cerr<<setw(5)<<it->first<<"  --->  ";
		for(set<int>::iterator sit = s.begin(); sit != s.end(); ++sit)
			cerr<<setw(5)<<*sit<<" ,";
		cerr<<endl;
	}
}

//should be called within Debug()
void ReorderedPosts::dumpTargetToSourceMap(){
	cerr<<"Source resolved reordered posts (target to source): "<<endl;
	for(map < int, set<int> > :: iterator it = targetToSourceMap.begin(); it != targetToSourceMap.end(); ++it){
		set<int> s = it->second;
		cerr<<setw(5)<<it->first<<"  --->  ";
		for(set<int>::iterator sit = s.begin(); sit != s.end(); ++sit)
			cerr<<setw(5)<<*sit<<" ,";
		cerr<<endl;
	}
}

void ReorderedPosts::dumpDependencies(){
	Debug(dumpSourceToTargetMap());
	Debug(dumpTargetToSourceMap());
}

map < int, set<int> > ReorderedPosts::getSourceToTargetMap(){
	return sourceToTargetMap;
}

map < int, set<int> > ReorderedPosts::getTargetToSourceMap(){
	return targetToSourceMap;
}

void ReorderedPosts::erase(){
	sourceToTargetMap.erase(sourceToTargetMap.begin(),sourceToTargetMap.end());
	targetToSourceMap.erase(targetToSourceMap.begin(),targetToSourceMap.end());
}

#endif /* REORDEREDPOSTS_H_ */
