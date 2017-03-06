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
 * This file contains the entry point for the EMDPOR algorithm and contains the
 * implementation for the algorithms Explore and BacktrackEager presented in the paper.
 *
 */

#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <time.h>
//#include <random>

//#include "./Headers/ReorderedPosts.h"
#include <ReorderedPosts.h>
#include <OperationProcessor.h>
#include <Sets.h>

using namespace std;
using namespace emdpor_preprocessor;
using namespace emdpor_OperationProcessor;

// Data Structure Declarations
map<int, Sets> mapOfStateToAllTheSets;			// < control location, Sets< backtrack set, sleep sets, done set > >
static bool isFirstOperation = true;
int maxIndexWhereComputeBacktrackFailed = -1;
int findtarget_fail_count = 0;
int findtarget_call_count = 0;
long eagerReplay_transition_count = 0;
int eagerReplay_call_count = 0;
long unoptimized_eagerReplay_transition_count = 0;
long replay_transition_count = 0;
long explored_transitions_count = 0;
int trace_count = 1;
int trace_index = 1;
clock_t tStart;
ReorderedPosts reorderedPosts;
//tracks the set of states where findTarget failed to add backtracking choices at identified backtracking states in the explored sequence.
set<int> findTargetFailed_state;

// Auxiliary Functions for "backtrackingSets"
Sets getSets(int controlLocation){
	map<int, Sets>::iterator it = mapOfStateToAllTheSets.find(controlLocation);
	Sets empty;
	if(it == mapOfStateToAllTheSets.end())
		return empty;
	return it->second;
}

void addSets(int controlLocation, Sets obj){
	mapOfStateToAllTheSets[controlLocation] = obj;
}


void dumpSets(){
	map<int, Sets>::iterator it = mapOfStateToAllTheSets.begin();
	if(it==mapOfStateToAllTheSets.end())
		return;
	for( ; it != mapOfStateToAllTheSets.end(); ++it){
		set<int> BTSet = it->second.getImmutableBacktrackingSet();
		set<int> doneSet = it->second.getImmutableDoneSet();

		cerr<<endl<<"Control Location   : "<<it->first<<endl<<"Backtracking Set   : ";
		for(set<int>::iterator vit = BTSet.begin(); vit != BTSet.end(); ++vit)
			cerr<<*vit<<", ";
		cerr<<endl<<"Done Set           : ";
		for(set<int>::iterator vit = doneSet.begin(); vit != doneSet.end(); ++vit)
			cerr<<*vit<<", ";
		cerr<<endl<<"has dpor run       : ";
		if(it->second.hasDporRun())
			cerr<<"true";
		else
			cerr<<"false";
		cerr<<endl<<"ReorderedPosts Set : ";
		for(set<pair<int,int> >::iterator vit = it->second.getImmutableReorderedPostSet().begin(); vit != it->second.getImmutableReorderedPostSet().end(); ++vit)
			cerr<<vit->first<<" --> "<<vit->second<<", ";
		cerr<<endl;
	}
}

void dumpSequence(){
	Debug(cerr<<"Dumping Sequence Below : "<<endl<<endl<<endl);
	Debug(cerr<<"CL     PI "<<endl);
	for(unsigned i =0; i<sequence.size(); ++i)
		Debug(cerr<<setw(2)<<i<<" --- "<<setw(2)<<sequence.at(i)<<endl);
}


void addOptimalBacktrackingChoice(int backtrackingPoint, set<int> threads){
	map<int, Sets>::iterator it = mapOfStateToAllTheSets.find(backtrackingPoint);
	if(it!=mapOfStateToAllTheSets.end()){
		//		it->second.getMutableBacktrackingSet().insert(*(threads.begin()));
		set<int> bktkSet = it->second.getImmutableBacktrackingSet();
		set<int> intersection = setIntersection(threads, bktkSet);
		if(!intersection.empty()){
			it->second.getMutableBacktrackingSet().insert(*(intersection.begin()));
		}
		else{
			it->second.getMutableBacktrackingSet().insert(*(threads.begin()));
		}
	}
	else{
		Sets obj;
		obj.getMutableBacktrackingSet().insert(*(threads.begin()));
		addSets(backtrackingPoint, obj);
	}
}


void addBacktrackingChoice(int backtrackingPoint, int threadId){
	//	Debug(cerr<<" addBacktrackingChoice() :: begin"<<endl);
	map<int, Sets>::iterator it = mapOfStateToAllTheSets.find(backtrackingPoint);
	if(it!=mapOfStateToAllTheSets.end())
		it->second.getMutableBacktrackingSet().insert(threadId);
	else{
		Sets obj;
		obj.getMutableBacktrackingSet().insert(threadId);
		addSets(backtrackingPoint, obj);
	}
	//	Debug(cerr<<" addBacktrackingChoice() :: end"<<endl);
}

void addIntoDoneSet(int backtrackingPoint, int threadId){
	//	Debug(cerr<<" addIntoDoneSet() :: begin"<<endl);
	map<int, Sets>::iterator it = mapOfStateToAllTheSets.find(backtrackingPoint);
	if(it!=mapOfStateToAllTheSets.end())
		it->second.getMutableDoneSet().insert(threadId);
	else{
		Sets obj;
		obj.getMutableDoneSet().insert(threadId);
		addSets(backtrackingPoint, obj);
	}

	//	Debug(cerr<<" addIntoDoneSet() :: end"<<endl);
}

//@warning: Should not be used. Only for testing purposes!
void eraseSets(){
	mapOfStateToAllTheSets.erase(mapOfStateToAllTheSets.begin(), mapOfStateToAllTheSets.end());
}


int getIndexAfterWhichPrune(){
	Debug(cerr<<"getIndexAfterWhichPrune() :: begin "<<endl);
	int indexAfterWhichPrune = -1;
	map<int, Sets>::reverse_iterator it = mapOfStateToAllTheSets.rbegin();
	for( ; it != mapOfStateToAllTheSets.rend(); ++it){

		set<int> ChoicesLeftSet = setDiff(it->second.getImmutableBacktrackingSet(), it->second.getImmutableDoneSet());
		if(!ChoicesLeftSet.empty()){
			indexAfterWhichPrune = it->first;
			break;
		}

	}
	Debug(cerr<<"Returning Index after which to prune : "<<indexAfterWhichPrune<<endl);
	return indexAfterWhichPrune;
}

void pruneSequence(){
	Debug(cerr<<"pruneSequence() :: begin "<<endl);
	int indexAfterWhichPrune = -1;
	map<int, Sets>::reverse_iterator it = mapOfStateToAllTheSets.rbegin();
	for( ; it != mapOfStateToAllTheSets.rend(); ++it){

		set<int> ChoicesLeftSet = setDiff(it->second.getImmutableBacktrackingSet(), it->second.getImmutableDoneSet());
		if(!ChoicesLeftSet.empty()){
			indexAfterWhichPrune = it->first;
			break;
		}
	}
	Debug(cerr<<" Index after which to prune : "<<indexAfterWhichPrune<<endl);
	if(indexAfterWhichPrune != -1){
		sequence.erase(sequence.begin()+indexAfterWhichPrune, sequence.end());
		pruneProgramIndexToControlLocationMap();
		map<int, Sets>::iterator it = mapOfStateToAllTheSets.find(indexAfterWhichPrune);
		++it;
		mapOfStateToAllTheSets.erase(it, mapOfStateToAllTheSets.end());
		//need to set dpor run status to false at backtracking point.
		if(!mapOfStateToAllTheSets.empty()){
			mapOfStateToAllTheSets.rbegin()->second.setDporRunStatus(false);
		}
	}
	else{
		sequence.erase(sequence.begin(), sequence.end());
		programIndexToControlLocationMap.erase(programIndexToControlLocationMap.begin(), programIndexToControlLocationMap.end());
		eraseSets();
		eraseClock();
	}
	Debug(cerr<<"pruneSequence() :: end"<<endl);
}

/*
// used to prune sequence till the point where computeBacktrackFailed last.
void pruneSequence(int index){
	Debug(cerr<<"overloaded pruneSequence(int) :: begin "<<endl);
	//	dumpHBGraph();
	Debug(cerr<<" Index after which to prune : "<<index<<";  sequence.size() : "<<sequence.size()<<endl);
	if(sequence.size() <= (unsigned)index) { cerr << "ERROR :: dporIndex should be less than size of sequence"<<endl; throw error;};
	if(index != -1){
		sequence.erase(sequence.begin()+index, sequence.end());
		pruneProgramIndexToControlLocationMap(index);
		map<int, Sets>::iterator it = mapOfStateToAllTheSets.find(index);
		++it;
		mapOfStateToAllTheSets.erase(it, mapOfStateToAllTheSets.end());
		//		HBGraphUtility::eraseHBGraph(index);
		//		indexToNodeMapper::eraseIndexToNodeMapper(index);
	}
	Debug(cerr<<"overloaded fn :: pruneSequence(int) :: end"<<endl);
}*/

///



// dumping external dependencies (mapped to control locations)
inline void dumpSourceToTargetDependenciesMap(map < int, set<int> > sourceToTargetDependenciesMap){
	Debug(cerr<<"Source to Target Map dump: "<<endl);
	for(map < int, set<int> > :: iterator it = sourceToTargetDependenciesMap.begin(); it != sourceToTargetDependenciesMap.end(); ++it){
		set<int> s = it->second;
		Debug(cerr<<setw(5)<<mapProgramIndexToControlLocation(it->first)<<"  --->  ");
		for(set<int>::iterator sit = s.begin(); sit != s.end(); ++sit)
			Debug(cerr<<setw(5)<<mapProgramIndexToControlLocation(*sit)<<" ,");
		Debug(cerr<<endl);
	}
}

inline void dumpTargetToSourceDependenciesMap(map < int, set<int> > targetToSourceDependenciesMap){
	Debug(cerr<<"Target to Source Map dump: "<<endl);
	for(map < int, set<int> > :: iterator it = targetToSourceDependenciesMap.begin(); it != targetToSourceDependenciesMap.end(); ++it){
		set<int> s = it->second;
		Debug(cerr<<setw(5)<<mapProgramIndexToControlLocation(it->first)<<"  --->  ");
		for(set<int>::iterator sit = s.begin(); sit != s.end(); ++sit)
			Debug(cerr<<setw(5)<<mapProgramIndexToControlLocation(*sit)<<" ,");
		Debug(cerr<<endl);
	}
}

inline void dumpDependencies(ExplicitDependencies obj){
	dumpSourceToTargetDependenciesMap(obj.getSourceToTargetMap());
	dumpTargetToSourceDependenciesMap(obj.getTargetToSourceMap());
}


// pre-processing function
bool preProcess(string traceFileName, string dependenceFileName){
	string line;
	ifstream traceFile (traceFileName.c_str());
	ifstream dependenceFile (dependenceFileName.c_str());
	if (traceFile.is_open() && dependenceFile.is_open())
	{
		while ( getline (traceFile,line) ){
			//			Debug(cerr<<line<<endl);
			Operation *op = processLine(line);
			if(op!=NULL)
				preprocessOperation(op);
		}
		if(!emdpor_preprocessor_private::preprocessingLockMap.empty()){
			map<long,int> aMap = emdpor_preprocessor_private::preprocessingLockMap;
			for(map<long,int>::iterator it = aMap.begin(); it!=aMap.end(); it++){
				cerr<<"ERROR::This lock operation never got unlocked in the program, lock id : "<<it->first<<", lock-op index : "<<it->second<<endl;
				throw error;
			}
		}
		if(!emdpor_preprocessor_private::preprocessingBadUnLockMap.empty()){
			map<long,int> aMap = emdpor_preprocessor_private::preprocessingBadUnLockMap;
			for(map<long,int>::iterator it = aMap.begin(); it!=aMap.end(); it++){
				cerr<<"ERROR::An unlock operation was found without corresponding lock operation at index :: "<<it->second<<endl;
			}
			throw error;
		}
		traceFile.close();
		deleteThreadStateMap();
		//		emdpor_preprocessor::dumpTrace();
		//		dumpthreadIndicesMap();
		//		dumptaskIndicesMap();

		while ( getline (dependenceFile,line) ){
			if(!processDependence(line))
				break;
		}
		dependenceFile.close();
		//		explicitDependencies.dumpDependencies();
		return true;
	}
	else cerr << "Unable to open file"<<endl;
	return false;
}


bool preProcess(const char* configFile){
	getConfig(configFile);
	string line;
	ifstream traceFile (traceFileName.c_str());
	ifstream dependenceFile (dependenceFileName.c_str());
	if (traceFile.is_open() && dependenceFile.is_open())
	{
		while ( getline (traceFile,line) ){
			Operation *op = processLine(line);
			if(op!=NULL)
				preprocessOperation(op);
		}
		if(!emdpor_preprocessor_private::preprocessingLockMap.empty()){
			map<long,int> aMap = emdpor_preprocessor_private::preprocessingLockMap;
			for(map<long,int>::iterator it = aMap.begin(); it!=aMap.end(); it++){
				cerr<<"ERROR::This lock operation never got unlocked in the program, lock id : "<<it->first<<", lock-op index : "<<it->second;
				throw error;
			}
		}
		if(!emdpor_preprocessor_private::preprocessingBadUnLockMap.empty()){
			map<long,int> aMap = emdpor_preprocessor_private::preprocessingBadUnLockMap;
			for(map<long,int>::iterator it = aMap.begin(); it!=aMap.end(); it++){
				cerr<<"ERROR::An unlock operation was found without corresponding lock operation at index :: "<<it->second<<endl;
			}
			throw error;
		}
		traceFile.close();
		deleteThreadStateMap();
		//		emdpor_preprocessor::dumpTrace();
		//		dumpthreadIndicesMap();
		//		dumptaskIndicesMap();

		while ( getline (dependenceFile,line) ){
			if(!processDependence(line))
				break;
		}
		dependenceFile.close();
		return true;
	}
	else cerr << "Unable to open file"<<endl;
	return false;
}


/* Handling ReorderedPosts.
 * call this method after argument operation has been executed.
 */
void addReorderedPostsHBEdges(Operation *op){
	if(op->getOpType()==POST || op->getOpType()==UI_POST || op->getOpType()==DELAY_POST || op->getOpType()==NATIVE_POST){
		set<pair<int,int> > HBEdges = reorderedPosts.processPost(mapContolLocationToProgramIndex(op->getOpIndex()));
		int threadId = op->getThreadId();
		//Due to the way targetToSourceMap field of ReorderedPosts class is implemented, HBEdges will only
		//contain one pair indicating the nearest reordered post w.r.t. op, thus adding the HB edge correctly as expected by the algorithm.
		for(set<pair<int,int> >::iterator it = HBEdges.begin(); it != HBEdges.end(); ++it){
			Debug(cerr<<"adding reordered posts HB Edge : "<<it->first<<"-->"<<it->second<<endl);
			int src = mapProgramIndexToControlLocation(it->first);
			int dest = mapProgramIndexToControlLocation(it->second);
			addHBEdge(getOperation(src), getOperation(dest), false);
			Debug(cerr<<"-----adding HB edges between reordered posts : " << src<<", "<<dest << ", BackTracking Point : " << src << endl);
			addBacktrackingChoice(src, threadId);
		}
	}
}

void resetReorderedPostsHBEdges(){
	reorderedPosts.erase();
	for(map<int, Sets>::iterator it = mapOfStateToAllTheSets.begin(); it != mapOfStateToAllTheSets.end(); ++it){
		set<pair<int,int> > rpSet = it->second.getImmutableReorderedPostSet();
		for(set<pair<int,int> >::iterator sit = rpSet.begin(); sit != rpSet.end(); ++sit){
			reorderedPosts.addPair(sit->first, sit->second);
		}
	}
}


Operation * getNextOperation(int threadId, int state){
	//Debug(cerr<<" Finding Next Operation for thread ID : "<<threadId<<endl);
	int nextInstIndex=-1;
	if(isFirstOperation == true){
		nextInstIndex = getNextInstructionForExecution(getThread(1), -1);
		isFirstOperation = false;
	}
	else{
		Thread t = getThreadState(state, threadId);
		if(t.isNullThread()){ cerr<<"ERROR :: Failed to get Thread Info in state "<< state<<" fn:getNextOperation()"<<endl; throw error;}
		map<int, vector<int> >::iterator it = threadIndicesMap.find(threadId);
		if(it != threadIndicesMap.end()){
			vector<int> threadIndices = it->second;
			if(!threadIndices.empty())
				nextInstIndex = getNextInstructionForExecution(t, *(threadIndices.rbegin()));
			else{
				cerr<<"ERROR :: in fn:isThreadEnabledForExecution(), threadIndices shouldn't have been empty!"<<endl;
				throw error;
			}
		}
		else
			nextInstIndex = getNextInstructionForExecution(t, state);
	}
	if(nextInstIndex == -1){
		//Debug(cerr<<"In fn:getNextOperation(), returning NULL for threadID : "<<threadId<<" State : "<<state<<endl);
		return NULL;
	}
	return getOperationClone(nextInstIndex);
}

void replay(bool isDPOR){
	trace_count++;
	trace_index++;
	Write(cerr<<"									 ---| TRACE - "<<trace_index<<" |----  "<<endl);
	Debug(cerr<<"replay() :: begin "<<endl);
	Debug(dumpSets());
	Debug(dumpSequence());
	for(unsigned i = 0; i < sequence.size(); i++){
		Operation *op = getOperationClone(sequence.at(i));
		op->setOpIndex(i);
		Debug(cerr<<endl<<"replaying control location__________________________ : "<<i<<" Operation : ");
		Debug(op->dumpOpInfo());
		Debug(cerr<<endl);
		replay_transition_count++;
		if( getSets(i).hasDporRun() )
			processOperation(op, true, true);
		else
			processOperation(op, isDPOR, true);
		explicitDependencies.resolveDependency(mapContolLocationToProgramIndex(op->getOpIndex()));
		addExplicitDependenciesState(op->getOpIndex(), explicitDependencies);
		addExplicitHBEdges(mapContolLocationToProgramIndex(op->getOpIndex()), op->getThreadId(), isDPOR);
		addReorderedPostsHBEdges(op);
	}
	Debug(cerr<<"replay() :: end "<<endl);
}

// a few forward declarations
void addBacktrackingChoicesDPOR(int state);
void addBacktrackingChoicesEmdpor_v2(int state);


/* This function is an implementation of BacktrackEager algorithm which will add backtracking choices
 * using dpor until dporIndex, rest of the sequence would be replayed using emdpor like computation
 * till emdporIndex.
 *
 *
 * Instead of performing BacktrackEager on seeing a failure, in the implementation we explore
 * the whole sequence, and log all states where findTarget failed and backtrackEager has to be invoked
 * to identify backtracking choices. After exploring the sequence, a single run
 * of BacktrackEager is performed to identify backtracking choices for all such logged pairs.
 * This implementation orders adjacent pairs of posts to the same thread and computes backtracking
 * choices to reorder posts like DPOR, upto the highest state index, dporIndex, at which FindTarget
 * failed to compute backtracking choices. From dporIndex till emdporIndex --- the highest operation
 * index which could not be reordered with a prior operation, backtracking choices are computed using
 * the extended HB relation (which also includes newly added adjacent post-to-post edges and corresponding FIFO)
 * but using FindTarget similar to EMDPOR. However, if this FindTarget fails to add backtracking choices then
 * we do not call BacktrackEager, rather similar to DPOR add all enabled threads at that point (because we
 * have already performed relevant BacktrackEager and accounted the adjacent post-to-post HB ordering).
 * Since this run re-explores already explored sequence but with an extended HB relation, we will not be
 * encountering any new backtrack failures than those already seen in the previous explored sequence.
 *
 */
void backtrackEager(int dporIndex, int emdporIndex){
	eagerReplay_call_count++;
	trace_index++;
	cerr<<"									 ---| TRACE (Adding Eager choices adding replay) - "<<trace_index<<" |----  "<<endl;
	Debug(cerr<<"overloaded replay(int) :: begin; dporIndex = "<< dporIndex <<"sequence.size() = "<< sequence.size() <<endl);
	if(sequence.size() < (unsigned)dporIndex) { cerr << "ERROR :: dporIndex should be less than size of sequence"<<endl; throw error;};
	Debug(dumpSets());
	Debug(dumpSequence());
	unsigned i;

	// this loop does replay with adding backtracking choices dpor way
	for(i = 0; i < (unsigned)dporIndex; i++){
		int state = i-1;

		if(state == -1){
			addThread(1);
		}

		addBacktrackingChoicesDPOR(state);

		Operation *op = getOperationClone(sequence.at(i));
		op->setOpIndex(i);
		Debug(cerr<<endl<<"replaying control location__________________________ : "<<i<<endl);
		Debug(op->dumpOpInfo());
		Debug(cerr<<endl);
		eagerReplay_transition_count++;
		// this function is always run with dpor, but need to reconstruct the HBGraph in this case, so passing isCalledFromReplay=false
		processOperation(op, true, false);
		Sets obj = getSets(i);
		obj.setDporRunStatus(true);
		addSets(i, obj);
		explicitDependencies.resolveDependency(mapContolLocationToProgramIndex(op->getOpIndex()));
		addExplicitDependenciesState(op->getOpIndex(), explicitDependencies);
		addExplicitHBEdges(mapContolLocationToProgramIndex(op->getOpIndex()), op->getThreadId(), false);
		addReorderedPostsHBEdges(op);
	}

	// this loop is for replay with emdpor
	for( ; i <= (unsigned)emdporIndex+1; i++){
		int state = i-1;

		if(configId==EMDPOR_MODE)
			addBacktrackingChoicesEmdpor_v2(state);

		Operation *op = getOperationClone(sequence.at(i));
		op->setOpIndex(i);
		Debug(cerr<<endl<<"replaying control location__________________________ : "<<i<<endl);
		eagerReplay_transition_count++;
		// this function is always run with dpor, but need to reconstruct the HBGraph in this case, so passing isCalledFromReplay=false
		processOperation(op, true, false);
		explicitDependencies.resolveDependency(mapContolLocationToProgramIndex(op->getOpIndex()));
		addExplicitDependenciesState(op->getOpIndex(), explicitDependencies);
		addExplicitHBEdges(mapContolLocationToProgramIndex(op->getOpIndex()), op->getThreadId(), false);
		addReorderedPostsHBEdges(op);
	}

	Debug(cerr<<"replay() :: end "<<endl);
}

void addBacktrackingChoicesDPOR(int state){
	Debug(cerr<<"addBacktrackingChoicesDPOR() :: begin"<<endl);
	if(state == -1)
		return;

	vector<int> enabledThreads = getAllThreads(state);

	while(! enabledThreads.empty() ){
		int threadId = enabledThreads.at(0);
		enabledThreads.erase(enabledThreads.begin());
		Operation * op = getNextOperation(threadId, state);

		if(op==NULL)
			continue;

		Debug(cerr<<"addBacktrackingChoicesDPOR() - operation : "<<enumOpName[op->getOpType()]<<endl);
		int opProgramIndex = op->getOpIndex();

		// Undo everything downwards after each iteration.

		op->setOpIndex(state+1);
		if(op->getOpType() == READ || op->getOpType() == READ_STATIC || op->getOpType() == WRITE || op->getOpType() == WRITE_STATIC
				|| op->getOpType() == LOCK || op->getOpType() == POST  || op->getOpType() == UI_POST  || op->getOpType() == NATIVE_POST
				|| op->getOpType() == DELAY_POST){
			Debug(cerr<<"/*/*/*/*/*/*/*/*/ Potential dependent operation : "<<enumOpName[op->getOpType()]<<" /*/*/*/*/*/*/*/*/"<<endl);
			int dependentOpWithNoHB = partialProcessOperation(op,true,opProgramIndex);		// passing true for DPOR
			Debug(cerr<<" index of dependentOpWithNoHB = "<<dependentOpWithNoHB<<endl);
			if(dependentOpWithNoHB != -1){
				//				pair< vector<int>, int> res = emdpor_backtrack::computeBacktrack(dependentOpWithNoHB, op, false);
				findtarget_call_count++;
				pair< set<int>, int> res = dpor_backtrack::computeBacktrack(dependentOpWithNoHB, op);

				set<int> resultSet = res.first;
				if(! resultSet.empty()){
					Debug(cerr<<"-----Backtracking Set is not empty, Thread to run : " << *(resultSet.begin()) << ", BackTracking Point : " << res.second << endl);

					// add optimization here : try to add the thread which is already there in backtracking choice.
					addOptimalBacktrackingChoice(res.second, resultSet);

					// older one - adding without optimization.
					//					addBacktrackingChoice(res.second, *(resultSet.begin()));
				}
				else{
					vector<int> enabledThreadsAtBTPoint = getEnabledThreadsInBacktrackCall(res.second-1);
					while(! enabledThreadsAtBTPoint.empty()){
						addBacktrackingChoice(res.second, enabledThreadsAtBTPoint.at(0));
						enabledThreadsAtBTPoint.erase(enabledThreadsAtBTPoint.begin());
					}
				}
			}

		}
		delete op;
	}
	Debug(cerr<<"addBacktrackingChoices() :: end"<<endl);
}


void addBacktrackingChoicesEmdpor(int state){
	Debug(cerr<<"addBacktrackingChoicesEmdpor() :: begin"<<endl);
	if(state == -1)
		return;

	vector<int> enabledThreads = getAllThreads(state);

	while(! enabledThreads.empty() ){
		int threadId = enabledThreads.at(0);
		enabledThreads.erase(enabledThreads.begin());
		Operation * op = getNextOperation(threadId, state);

		if(op==NULL)
			continue;

		// Undo everything downwards after each iteration.
		int opProgramIndex = op->getOpIndex();
		op->setOpIndex(state+1);

		if(op->getOpType() == WRITE || op->getOpType() == WRITE_STATIC || op->getOpType() == LOCK){
			Debug(cerr<<"/*/*/*/*/*/*/*/*/ Potential dependent operation : "<<enumOpName[op->getOpType()]<<" /*/*/*/*/*/*/*/*/"<<endl);
			set<int> dependentOpsWithNoHB = partialProcessWriteAndLockOperations(op, false, opProgramIndex);			// passing false for emdpor
			bool firstIteration = true;
			for(set<int>::reverse_iterator setRevIterator = dependentOpsWithNoHB.rbegin();
					setRevIterator != dependentOpsWithNoHB.rend(); setRevIterator++){
				int dependentOpWithNoHB = *setRevIterator;
				Debug(cerr<<" index of dependentOpWithNoHB = "<<dependentOpWithNoHB<<endl);
				if(dependentOpWithNoHB != -1){
					findtarget_call_count++; //collected for the purpose of reporting stats
					set< pair< set<int>, int> > result;
					/* In the algorithm presented in the paper, backtracking choices are added at the right state
					 * by FindTarget Algorithm itself. However, the implementation of FindTarget here computes
					 * the set of backtracking choices and the backtracking points and returns it. The actual
					 * addition of backtracking choices to states happens outside findTarget.
					 */
					result = emdpor_backtrack::findTarget(dependentOpWithNoHB, op, mapOfStateToAllTheSets);

					if(firstIteration){
						//for intuition behind this, check extended paper which presents optimizations to EMDPOR algorithm.
						addTempHBEdge(getOperation(dependentOpWithNoHB), op, false);
						firstIteration = false;
					}

					for(set< pair< set<int>, int> >::iterator complexItr = result.begin(); complexItr != result.end(); ++complexItr){
						pair< set<int>, int> res = *complexItr;

						set<int> resultSet = res.first;
						if(! resultSet.empty()){
							Debug(cerr<<"-----Backtracking Set is not empty, Thread to run : " << *(resultSet.begin()) << ", BackTracking Point : " << res.second << endl);

							// This is an optimization taken from DPOR paper [Flanagan & Godefroid, POPL 2005]
							addOptimalBacktrackingChoice(res.second, resultSet);
						}
						else{
							Debug(cerr<<"-----Backtracking Set is empty, BackTracking Point : " << res.second << endl);

							/* Instead of performing BacktrackEager on seeing a failure, in the implementation we explore
							 * the whole sequence, and log all states where findTarget failed and backtrackEager has to be invoked
							 * to identify backtracking choices. After exploring the sequence, a single run
							 * of BacktrackEager is performed to identify backtracking choices for all such logged pairs.
							 */
							findtarget_fail_count++; //collecting some stats
							Debug(cerr<<"#findTarget failed = "<< findtarget_fail_count<<endl);

							unoptimized_eagerReplay_transition_count+=res.second; //collecting stats related to number of transitions to be re-executed to do BacktrackEager

							if(res.second > maxIndexWhereComputeBacktrackFailed){
								maxIndexWhereComputeBacktrackFailed = res.second;
								Debug(cerr<<" maxIndexWhereComputeBacktrackFailed : " << maxIndexWhereComputeBacktrackFailed << endl);
							}
							findTargetFailed_state.insert(state);
						}
					}
				}
			}
			/* should be removed since op is not yet executed in the sequence. The HB edge was temporarily added
			 * to only aid in identifying backtracking choices.
			 */
			removeTempHBEdge();
		}

		else if(op->getOpType() == READ || op->getOpType() == READ_STATIC){
			Debug(cerr<<"/*/*/*/*/*/*/*/*/ Potential dependent operation : "<<enumOpName[op->getOpType()]<<" /*/*/*/*/*/*/*/*/"<<endl);
			int dependentOpWithNoHB = partialProcessOperation(op, false, opProgramIndex);			// passing false for emdpor
			Debug(cerr<<" index of dependentOpWithNoHB = "<<dependentOpWithNoHB<<endl);
			if(dependentOpWithNoHB != -1){
				findtarget_call_count++;	//collecting some stats
				set< pair< set<int>, int> > result;
				result = emdpor_backtrack::findTarget(dependentOpWithNoHB, op, mapOfStateToAllTheSets);

				for(set< pair< set<int>, int> >::iterator complexItr = result.begin(); complexItr != result.end(); ++complexItr){
					pair< set<int>, int> res = *complexItr;

					set<int> resultSet = res.first;
					if(! resultSet.empty()){
						Debug(cerr<<"-----Backtracking Set is not empty, Thread to run : " << *(resultSet.begin()) << ", BackTracking Point : " << res.second << endl);

						// add optimization here : try to add the thread which is already there in backtracking choice.
						addOptimalBacktrackingChoice(res.second, resultSet);
					}
					else{
						Debug(cerr<<"-----Backtracking Set is empty, BackTracking Point : " << res.second << endl);

						/* Instead of performing BacktrackEager on seeing a failure, in the implementation we explore
						 * the whole sequence, and log all states where findTarget failed and backtrackEager has to be invoked
						 * to identify backtracking choices. After exploring the sequence, a single run
						 * of BacktrackEager is performed to identify backtracking choices for all such logged pairs.
						 */
						findtarget_fail_count++;
						Debug(cerr<<"#findTarget failed = "<< findtarget_fail_count<<endl);

						unoptimized_eagerReplay_transition_count+=res.second;

						if(res.second > maxIndexWhereComputeBacktrackFailed){
							maxIndexWhereComputeBacktrackFailed = res.second;
							Debug(cerr<<" maxIndexWhereComputeBacktrackFailed : " << maxIndexWhereComputeBacktrackFailed << endl);
						}

						findTargetFailed_state.insert(state);
					}
				}
			}
		}
		delete op;
	}
	Debug(cerr<<"addBacktrackingChoices() :: end"<<endl);
}

// this one should be called from backtrackEager(dpor_until_i, emdpor_until_j)
void addBacktrackingChoicesEmdpor_v2(int state){
	Debug(cerr<<"addBacktrackingChoicesEmdpor() :: begin"<<endl);
	if(state == -1)
		return;

	vector<int> enabledThreads = getAllThreads(state);

	while(! enabledThreads.empty() ){
		int threadId = enabledThreads.at(0);
		enabledThreads.erase(enabledThreads.begin());
		Operation * op = getNextOperation(threadId, state);

		if(op==NULL)
			continue;

		// Undo everything downwards after each iteration.
		int opProgramIndex = op->getOpIndex();
		op->setOpIndex(state+1);

		if(op->getOpType() == WRITE || op->getOpType() == WRITE_STATIC || op->getOpType() == LOCK){
			Debug(cerr<<" Potential dependent operation : "<<enumOpName[op->getOpType()]<<endl);
			set<int> dependentOpsWithNoHB = partialProcessWriteAndLockOperations(op, false, opProgramIndex);			// passing false for emdpor
			bool firstIteration = true;
			for(set<int>::reverse_iterator setRevIterator = dependentOpsWithNoHB.rbegin();
					setRevIterator != dependentOpsWithNoHB.rend(); setRevIterator++){
				int dependentOpWithNoHB = *setRevIterator;
				Debug(cerr<<" index of dependentOpWithNoHB = "<<dependentOpWithNoHB<<endl);
				if(dependentOpWithNoHB != -1){
					findtarget_call_count++;
					set< pair< set<int>, int> > result;
					result = emdpor_backtrack::findTarget(dependentOpWithNoHB, op, mapOfStateToAllTheSets);

					if(firstIteration){
						addTempHBEdge(getOperation(dependentOpWithNoHB), op, false);
						firstIteration = false;
					}

					for(set< pair< set<int>, int> >::iterator complexItr = result.begin(); complexItr != result.end(); ++complexItr){
						pair< set<int>, int> res = *complexItr;

						set<int> resultSet = res.first;
						if(! resultSet.empty()){
							Debug(cerr<<"-----Backtracking Set is not empty, Thread to run : " << *(resultSet.begin()) << ", BackTracking Point : " << res.second << endl);

							// add optimization here : try to add the thread which is already there in backtracking choice.
							addOptimalBacktrackingChoice(res.second, resultSet);
						}
						else{
							Debug(cerr<<"-----Backtracking Set is empty, BackTracking Point : " << res.second << endl);
							findtarget_fail_count++;
							Debug(cerr<<"#findTarget failed = "<< findtarget_fail_count<<endl);
							unoptimized_eagerReplay_transition_count+=res.second;
							vector<int> enabledThreadsAtBTPoint = getEnabledThreadsInBacktrackCall(res.second-1);
							Debug(cerr<<"-----Adding all Enabled Threads, no. of enabled threads : " << enabledThreadsAtBTPoint.size() << endl);
							while(! enabledThreadsAtBTPoint.empty()){
								addBacktrackingChoice(res.second, enabledThreadsAtBTPoint.at(0));
								enabledThreadsAtBTPoint.erase(enabledThreadsAtBTPoint.begin());
							}
						}
					}
				}
			}
			removeTempHBEdge();
		}

		else if(op->getOpType() == READ || op->getOpType() == READ_STATIC){
			Debug(cerr<<"/*/*/*/*/*/*/*/*/ Potential dependent operation : "<<enumOpName[op->getOpType()]<<" /*/*/*/*/*/*/*/*/"<<endl);
			//			dumpHBGraph();
			int dependentOpWithNoHB = partialProcessOperation(op, false, opProgramIndex);			// passing false for emdpor
			Debug(cerr<<" index of dependentOpWithNoHB = "<<dependentOpWithNoHB<<endl);
			if(dependentOpWithNoHB != -1){
				findtarget_call_count++;
				set< pair< set<int>, int> > result;
				result = emdpor_backtrack::findTarget(dependentOpWithNoHB, op, mapOfStateToAllTheSets);

				for(set< pair< set<int>, int> >::iterator complexItr = result.begin(); complexItr != result.end(); ++complexItr){
					pair< set<int>, int> res = *complexItr;

					set<int> resultSet = res.first;
					if(! resultSet.empty()){
						Debug(cerr<<"-----Backtracking Set is not empty, Thread to run : " << *(resultSet.begin()) << ", BackTracking Point : " << res.second << endl);

						// add optimization here : try to add the thread which is already there in backtracking choice.
						addOptimalBacktrackingChoice(res.second, resultSet);

					}
					else{
						Debug(cerr<<"-----Backtracking Set is empty, BackTracking Point : " << res.second << endl);

						findtarget_fail_count++;
						Debug(cerr<<"#findTarget failed = "<< findtarget_fail_count<<endl);

						unoptimized_eagerReplay_transition_count+=res.second;

						//					if(res.second > maxIndexWhereComputeBacktrackFailed){
						//						maxIndexWhereComputeBacktrackFailed = res.second;
						//						Debug(cerr<<" maxIndexWhereComputeBacktrackFailed : " << maxIndexWhereComputeBacktrackFailed << endl);
						//					}
						vector<int> enabledThreadsAtBTPoint = getEnabledThreadsInBacktrackCall(res.second-1);
						Debug(cerr<<"-----Adding all Enabled Threads, no. of enabled threads : " << enabledThreadsAtBTPoint.size() << endl);
						while(! enabledThreadsAtBTPoint.empty()){
							addBacktrackingChoice(res.second, enabledThreadsAtBTPoint.at(0));
							enabledThreadsAtBTPoint.erase(enabledThreadsAtBTPoint.begin());
						}
					}
				}
			}
		}
		delete op;
	}
	Debug(cerr<<"addBacktrackingChoices() :: end"<<endl);
}

//whenever this is called, it should return a value grater than -1.
int getMax_j_forWhichFindTargetFailed(){
	Debug(cerr<<"fn : getMax_j_forWhichFindTargetFailed() :: beginning"<<endl);
	int maxJ = -1;

//	dumpSequence();

	for(set<int>::iterator it = findTargetFailed_state.begin(); it != findTargetFailed_state.end(); ++it){
		Debug(cerr<<"state : "<<*it<<endl);
		if(*it > maxJ)
			maxJ = *it;
	}
	if(maxJ == -1){
		cerr<<"ERROR :: fn: getMax_j_forWhichFindTargetFailed() returning -1"<<endl;
		throw error;
	}
	Debug(cerr<<"fn : getMax_j_forWhichFindTargetFailed() :: end, return value - "<<maxJ<<endl);
	return maxJ;
}

/*
 * executes input trace as first sequence.
 */
void executeProgram(int configId){
	Debug(cerr<<"executeProgram() :: begin ");
	Debug(cerr<<" programLength : "<<programLength<<endl);
	map<int, Operation *> :: iterator it = programCode.begin();
	int state = -1;
	for( ; it != programCode.end(); ++it,++state){
		Operation *op = it->second->getClone();

		if(state == -1){
			addThread(1);
			Write(cerr<<"									 ---| TRACE - 1 |----  "<<endl);
		}

		if(state>0){
			if(configId==DPOR_MODE){
				addBacktrackingChoicesDPOR(state);
			}
			else if(configId==EMDPOR_MODE){
				addBacktrackingChoicesEmdpor(state);
			}
		}
		if(op == NULL) {cerr<<"Error :: in fn:execute() getNextOperation returned NULL"<<endl; throw error;}
		Debug(cerr<<endl<<endl<<"--------------------------------------------------------------------------------------------State is : "<<state<<endl);
		Debug(op->dumpOpInfo());

		int programIndex = op->getOpIndex();
		addIntoSequence(programIndex, state+1);
		addBacktrackingChoice(state+1, op->getThreadId());//the first thread explored from a state should trivially be in backtracking set
		addIntoDoneSet(state+1, op->getThreadId());
		op->setOpIndex(state+1);
		explored_transitions_count++;	//collecting some stats

		if(configId==DPOR_MODE)
			processOperation(op, true, false);
		else
			processOperation(op, false, false);
		explicitDependencies.resolveDependency(programIndex);
		addExplicitDependenciesState(state+1, explicitDependencies);
		if(configId==DPOR_MODE)
			addExplicitHBEdges(programIndex, op->getThreadId(),true);
		else{
			//As given in the Explore Algorithm (in the paper) EMDPOR adds HB edges between nearest pairs of reordered posts
			addReorderedPostsHBEdges(op);
			addExplicitHBEdges(programIndex, op->getThreadId(),false);
		}
	}
	Debug(cerr<<"executeProgram() :: End ");
}

inline void resetDataStructuresBeforeReplay(){
//	Debug(dumpHBGraph());
	eraseBookKeeper();
	eraseClock();
	pruneSequence();
	resetExplicitDependencies();
	resetReorderedPostsHBEdges();
	findTargetFailed_state.erase(findTargetFailed_state.begin(), findTargetFailed_state.end());
//	Debug(dumpHBGraph());
}

inline void resetDataStructuresBeforeReplay(int pruneIndex){
//	Debug(dumpHBGraph());
	eraseBookKeeper();
	eraseClock();
//	pruneSequence(pruneIndex);						// here is the only change in this overload
	resetExplicitDependencies();
	resetReorderedPostsHBEdges();
	maxIndexWhereComputeBacktrackFailed = -1;
//	Debug(dumpHBGraph());
}

bool execute(int state, vector<int> enabledThreads, bool isDPOR){
	Debug(cerr<<"execute() begin"<<endl);
	int threadIdToExecute;
	Sets setsObject =  getSets(state+1);
	set<int> choicesLeftSet;

	if(!enabledThreads.empty()){
		choicesLeftSet = setDiff(setsObject.getImmutableBacktrackingSet(), setsObject.getImmutableDoneSet());

		// here take intersection of choicesLeft with enabled threads, so that some choice that is not enabled in this run gets removed.
		choicesLeftSet = setIntersection(choicesLeftSet, getSetForVector(enabledThreads));

		if(choicesLeftSet.empty()){
		    threadIdToExecute = *(getSetForVector(enabledThreads).begin());
		    addBacktrackingChoice(state+1, threadIdToExecute);
		}
		else{
			set<int>::iterator iit = choicesLeftSet.begin();
			//			int randomChoice = scheduler(choicesLeftSet.size());
			//			for(int ii = 0; ii<randomChoice; ii++)
			//				++iit;
			threadIdToExecute = *iit;
		}
		addIntoDoneSet(state+1, threadIdToExecute);
		Operation * op = getNextOperation(threadIdToExecute, state);
		if(op == NULL) {cerr<<"Error :: in fn:execute() getNextOperation returned NULL"<<endl; throw error;}
		Debug(cerr<<endl<<endl<<"--------------------------------------------------------------------------------------------State is : "<<state<<endl);
		Debug(op->dumpOpInfo());
		//		dumpHBGraph();
		int programIndex = op->getOpIndex();
		addIntoSequence(programIndex, state+1);
		op->setOpIndex(state+1);
		explored_transitions_count++;
		processOperation(op, isDPOR, false);
		explicitDependencies.resolveDependency(programIndex);
		addExplicitDependenciesState(state+1, explicitDependencies);
		if(configId==DPOR_MODE)
			addExplicitHBEdges(programIndex, op->getThreadId(),true);
		else{
			addReorderedPostsHBEdges(op);
			addExplicitHBEdges(programIndex, op->getThreadId(),false);
		}

		return false;
	}
	else{
		Debug(cerr<<"execute() :: chk 4"<<endl);
		return true;
	}
}


void reportProgramDetails(){
	ofstream reportfile;
	reportfile.open (reportFilePath.c_str(), ios::app);
	reportfile<<"trace file path : "<<traceFileName<<endl;
	reportfile<<"program length : "<<programLength<<endl;
	reportfile<<"No. of threads : "<<emdpor_preprocessor_private::getThreadCount()<<endl;
	reportfile<<"No. of tasks : "<<emdpor_preprocessor_private::getTaskCount()<<endl;
	reportfile.close();
}

bool writeReport(clock_t lastReportTime, bool isProgramTerminating){
	ofstream reportfile;
	clock_t now = clock();
	double gap = (double) (now - lastReportTime) / (CLOCKS_PER_SEC * 60);
	//	cerr<<"gap : "<<gap<<endl;

	if(gap < frequencyInMins && !isProgramTerminating)
		return false;

	reportfile.open (reportFilePath.c_str(), ios::app);

	if(isProgramTerminating)
		reportfile<<endl<<endl<<"--------------- Program Terminated Successfully, Below is the final summary ---------------"<<endl;

	reportfile<<endl<<endl<<"#findTarget called \t\t\t"<< findtarget_call_count << endl;
	reportfile<<"#explored transitions \t\t\t"<< explored_transitions_count << endl;
	reportfile<<"#replayed transitions \t\t\t"<< replay_transition_count << endl;
	reportfile<<"#traces explored \t\t\t"<< trace_count << endl;
	reportfile<<"Time taken \t\t\t\t"<< (double)(clock() - tStart)/CLOCKS_PER_SEC<<endl;
	reportfile<<"reschedulePending Worklist Counter \t"<< emdpor_backtrack::reschedulePendingWorklistCounter << endl;

	if(configId==EMDPOR_MODE){
		reportfile<<"#findTarget failed \t\t\t"<< findtarget_fail_count << endl;
		reportfile<<"#eagerly-replayed transitions \t\t"<< eagerReplay_transition_count << endl;
//		reportfile<<"#eagerReplay called \t\t\t"<< eagerReplay_call_count << endl;
//		reportfile<<"#unoptimized-eagerReplayed transitions \t"<< unoptimized_eagerReplay_transition_count <<endl;
	}

//	if(!isProgramTerminating){
//		int backtracking_choices_left=0;
//		map<int,Sets>::iterator it = mapOfStateToAllTheSets.begin();
//		for( ; it!=mapOfStateToAllTheSets.end(); ++it){
//			set<int> temp = setDiff(it->second.getImmutableBacktrackingSet(), it->second.getImmutableDoneSet());
//			backtracking_choices_left+=temp.size();
//		}
//		reportfile<<"#backtracking_choices_left \t\t"<< backtracking_choices_left << endl;
//	}

	reportfile.close();
	return true;
}


void explore(int configId){
	bool continueExecution = true;
	bool isReplayRequired = false;

	executeProgram(configId);

	int maxNonEmptybktkChoiceIndex = getIndexAfterWhichPrune();

	if(maxIndexWhereComputeBacktrackFailed == -1){
		Debug(cerr<<"max NonEmpty bktkChoice Index : "<< maxNonEmptybktkChoiceIndex <<endl);
		resetDataStructuresBeforeReplay();
		if(sequence.empty()){
			Debug(cerr<<"Found Empty Sequence after first run, Terminating Program !"<<endl);
			return;
		}
		else
			isReplayRequired=true;
	}
	else{
		Debug(cerr<<"max index where computeBacktrack failed : "<< maxIndexWhereComputeBacktrackFailed <<endl);
		int replayWithDPOR_until_i_Value = maxIndexWhereComputeBacktrackFailed;
		int replayWithEmdpor_until_j_Value = getMax_j_forWhichFindTargetFailed();
		// run dpor to add eager choices till fail point
		resetDataStructuresBeforeReplay(replayWithEmdpor_until_j_Value);
		backtrackEager(replayWithDPOR_until_i_Value,replayWithEmdpor_until_j_Value);

		//now prune sequence to last state where (backtrack \ done set) is non empty
		resetDataStructuresBeforeReplay();
		if(sequence.empty()){
			Debug(cerr<<"Found Empty Sequence in Replay(), Terminating Program !"<<endl);
			return;
		}
		else
			isReplayRequired=true;
	}

	clock_t startTime;
	Report(startTime = clock());
	while(continueExecution){
		Report(if(writeReport(startTime,false))
			startTime = clock());

		if(isReplayRequired){
			if(configId==DPOR_MODE)
				replay(true);
			else
				replay(false);
		}
		int state = getLastControlLocation();
		vector<int> enabledThreads;
		if(state == -1){
			addThread(1);
			enabledThreads.push_back(1);
			Write(cerr<<"									 ---| TRACE - 1 |----  "<<endl);
		}
		else{
			Debug(cerr<<endl<<endl);
			enabledThreads = getEnabledThreads(state);
		}

		if(configId==DPOR_MODE){
			addBacktrackingChoicesDPOR(state);
			isReplayRequired = execute(state, enabledThreads, true);
		}
		else if(configId==EMDPOR_MODE){
			addBacktrackingChoicesEmdpor(state);
			isReplayRequired = execute(state, enabledThreads, false);
		}

		if(isReplayRequired){
			maxNonEmptybktkChoiceIndex = getIndexAfterWhichPrune();


			if(maxIndexWhereComputeBacktrackFailed == -1){
				Debug(cerr<<"max NonEmpty bktkChoice Index : "<< maxNonEmptybktkChoiceIndex <<endl);
				resetDataStructuresBeforeReplay();
				if(sequence.empty()){
					Debug(cerr<<"Found Empty Sequence in Replay(), Terminating Program !"<<endl);
					continueExecution = false;
				}
			}
			else{
				Debug(cerr<<"max index where computeBacktrack failed : "<< maxIndexWhereComputeBacktrackFailed <<endl);
				int replayWithDPOR_until_i_Value = maxIndexWhereComputeBacktrackFailed;
				int replayWithEmdpor_until_j_Value = getMax_j_forWhichFindTargetFailed();
				// run dpor to add eager choices till fail point
				resetDataStructuresBeforeReplay(replayWithEmdpor_until_j_Value);
				backtrackEager(replayWithDPOR_until_i_Value,replayWithEmdpor_until_j_Value);

				//now prune sequence to last state where (backtrack \ done set) is non empty
				resetDataStructuresBeforeReplay();
				if(sequence.empty()){
					Debug(cerr<<"Found Empty Sequence in Replay(), Terminating Program !"<<endl);
					continueExecution = false;
				}
			}
		}
	}
}



int main(int argc, char *argv[])
{
	tStart = clock();
	try{

		if(argc!=2){
			cerr<<"Only 1 argument expected -- path to the configuration file config.cfg"<<endl;
			throw error;
		}

		if(!preProcess(argv[1])){
			cerr<<"main() : ERROR :: preProcess() failed!!"<<endl;
			throw error;
		}
		Debug(cerr<<"programLength : "<<programLength<<endl);

		// setting configuration to run only emdpor in this repo branch
		configId=EMDPOR_MODE;

		if(configId==DPOR_MODE){
			initClock(programLength, getMaxThreadId(), getMaxTaskId(), true);
			reportProgramDetails();
			explore(DPOR_MODE);
			writeReport(tStart,true);
			cerr<<endl<<endl<<"\t\t---- DPOR + Vector Clock ----"<<endl<<endl;
			cerr<<"#findTarget called \t\t\t"<< findtarget_call_count << endl;

			//(replays not included)
			cerr<<"#explored transitions \t\t\t"<< explored_transitions_count << endl;

			cerr<<"#replayed transitions \t\t\t"<< replay_transition_count << endl;
			cerr<<"#traces explored \t\t\t"<< trace_count << endl;
			cerr<<"Time taken \t\t\t\t"<< (double)(clock() - tStart)/CLOCKS_PER_SEC<<endl;
		}
		else if(configId==EMDPOR_MODE){
			initClock(programLength, getMaxThreadId(), getMaxTaskId(), false);
			reportProgramDetails();
			explore(EMDPOR_MODE);
			writeReport(tStart,true);
			cerr<<endl<<endl<<"\t\t---- EM-DPOR ----"<<endl<<endl;

			// with dpor?? - yes
			cerr<<"#findTarget called \t\t\t"<< findtarget_call_count << endl;

			//(replays not included)
			cerr<<"#explored transitions \t\t\t"<< explored_transitions_count << endl;

			//(replay without adding ones caused by findTarget failure)
			cerr<<"#replayed transitions \t\t\t"<< replay_transition_count << endl;

			cerr<<"#traces explored \t\t\t"<< trace_count << endl;

			cerr<<"Time taken \t\t\t\t"<< (double)(clock() - tStart)/CLOCKS_PER_SEC<<endl;

			// only for emdpor
			cerr<<"#findTarget failed \t\t\t"<< findtarget_fail_count << endl;

			//(due to findTarget failure)
			cerr<<"#eagerly-replayed transitions \t\t"<< eagerReplay_transition_count << endl;

			//optimization effects
//			cerr<<"#eagerReplay called \t\t\t"<< eagerReplay_call_count << endl;
//			cerr<<"#unoptimized-eagerReplayed transitions \t"<< unoptimized_eagerReplay_transition_count <<endl;
		}
		deleteClock();
	}
	catch (exception& e)
	{
		cerr << e.what() << '\n';
		if(programLength!=0)
			deleteClock();
		cout<<1;
		return 1;
	}
	cout<<0;
	return 0;
}
