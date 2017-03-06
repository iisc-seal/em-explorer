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
 * This file contains the entry point for the DPOR algorithm and contains the
 * implementation for the algorithm Explore.
 *
 */

#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <time.h>
//#include <random>

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
long unoptimized_eagerReplay_transition_count = 0;
int eagerReplay_call_count = 0;
long replay_transition_count = 0;
long explored_transitions_count = 0;
int trace_count = 1;
int trace_index = 1;
clock_t tStart;

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

		set<int> diff = setDiff(BTSet, doneSet);
		if(diff.empty())
			continue;

		cerr<<endl<<"Control Location : "<<it->first<<endl<<"Backtracking Set : ";
		for(set<int>::iterator vit = BTSet.begin(); vit != BTSet.end(); ++vit)
			cerr<<*vit<<", ";
		cerr<<endl<<"Done Set         : ";
		for(set<int>::iterator vit = doneSet.begin(); vit != doneSet.end(); ++vit)
			cerr<<*vit<<", ";
		cerr<<endl<<"has dpor run     : ";
		if(it->second.hasDporRun())
			cerr<<"true";
		else
			cerr<<"false";
		cerr<<endl;
	}
}

void dumpSequence(){
	cerr<<"Dumping Sequence Below : "<<endl<<endl<<endl;
	cerr<<"CL     PI "<<endl;
	for(unsigned i =0; i<sequence.size(); ++i)
		cerr<<setw(2)<<i<<" --- "<<setw(2)<<sequence.at(i)<<endl;
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

// dumping external dependencies (mapped to control locations)

inline void dumpSourceToTargetDependenciesMap(map < int, set<int> > sourceToTargetDependenciesMap){
	cerr<<"Source to Target Map dump: "<<endl;
	for(map < int, set<int> > :: iterator it = sourceToTargetDependenciesMap.begin(); it != sourceToTargetDependenciesMap.end(); ++it){
		set<int> s = it->second;
		cerr<<setw(5)<<mapProgramIndexToControlLocation(it->first)<<"  --->  ";
		for(set<int>::iterator sit = s.begin(); sit != s.end(); ++sit)
			cerr<<setw(5)<<mapProgramIndexToControlLocation(*sit)<<" ,";
		cerr<<endl;
	}
}

inline void dumpTargetToSourceDependenciesMap(map < int, set<int> > targetToSourceDependenciesMap){
	cerr<<"Target to Source Map dump: "<<endl;
	for(map < int, set<int> > :: iterator it = targetToSourceDependenciesMap.begin(); it != targetToSourceDependenciesMap.end(); ++it){
		set<int> s = it->second;
		cerr<<setw(5)<<mapProgramIndexToControlLocation(it->first)<<"  --->  ";
		for(set<int>::iterator sit = s.begin(); sit != s.end(); ++sit)
			cerr<<setw(5)<<mapProgramIndexToControlLocation(*sit)<<" ,";
		cerr<<endl;
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
	else Debug(cerr << "Unable to open file"<<endl);
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
			//			Debug(cerr<<line<<endl);
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
		//		explicitDependencies.dumpDependencies();
		return true;
	}
	else Debug(cerr << "Unable to open file"<<endl);
	return false;
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
	Write(cerr<<"									 ---| TRACE - "<<trace_index<<" |----  "<<endl;
	cerr<<"replay() :: begin "<<endl);
	Debug(dumpSets());
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
		addExplicitHBEdges(mapContolLocationToProgramIndex(op->getOpIndex()), op->getThreadId());
	}
	Debug(cerr<<"replay() :: end "<<endl);
}

// a few forward declarations
void addBacktrackingChoicesDPOR(int state);
void addBacktrackingChoicesEmdpor_v2(int state, bool isFindTargetCallRecursive);

// will add backtracking choices using dpor until dporIndex, rest of the sequence would be replayed without adding backtracking point ( like in replay() )
void backtrackEager(int dporIndex, int emdporIndex){
	eagerReplay_call_count++;
	trace_index++;
	cerr<<endl<<"									 ---| TRACE (Adding Eager choices adding replay) - "<<trace_index<<" |----  "<<endl;
	Debug(cerr<<"overloaded replay(int) :: begin; dporIndex = "<< dporIndex <<"sequence.size() = "<< sequence.size() <<endl);
	if(sequence.size() < (unsigned)dporIndex) { cerr << "ERROR :: dporIndex should be less than size of sequence"<<endl; throw error;};
	Debug(dumpSets());
	unsigned i;

	// this loop does replay with adding backtracking choices dpor way
	for(i = 0; i < (unsigned)dporIndex; i++){
		int state = i-1;
		vector<int> enabledThreads;
		if(state == -1){
			addThread(1);
			enabledThreads.push_back(1);
		}
		else
			enabledThreads = getEnabledThreads(state);

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
		//		Debug(dumpDependencies(explicitDependencies));
		explicitDependencies.resolveDependency(mapContolLocationToProgramIndex(op->getOpIndex()));
		addExplicitDependenciesState(op->getOpIndex(), explicitDependencies);
		addExplicitHBEdges(mapContolLocationToProgramIndex(op->getOpIndex()), op->getThreadId());
	}

	// this loop is for replay with EmDpor
	for( ; i <= (unsigned)emdporIndex+1; i++){
		int state = i-1;

		if(configId==EMDPOR_MODE)
			addBacktrackingChoicesEmdpor_v2(state, true);

		Operation *op = getOperationClone(sequence.at(i));
		op->setOpIndex(i);
		Debug(cerr<<endl<<"replaying control location__________________________ : "<<i<<endl);
		//		Debug(op->dumpOpInfo());
		//		Debug(cerr<<endl);
		eagerReplay_transition_count++;
		// this function is always run with dpor, but need to reconstruct the HBGraph in this case, so passing isCalledFromReplay=false
		processOperation(op, true, false);
		//		Debug(dumpDependencies(explicitDependencies));
		explicitDependencies.resolveDependency(mapContolLocationToProgramIndex(op->getOpIndex()));
		addExplicitDependenciesState(op->getOpIndex(), explicitDependencies);
		addExplicitHBEdges(mapContolLocationToProgramIndex(op->getOpIndex()), op->getThreadId());
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

		Debug(cerr<<"addBacktrackingChoicesDPOR() - operation : "<<enumOpName[op->getOpId()]<<endl);
		int opProgramIndex = op->getOpIndex();

		// Undo everything downwards after each iteration.

		op->setOpIndex(state+1);
		if(op->getOpId() == READ || op->getOpId() == READ_STATIC || op->getOpId() == WRITE || op->getOpId() == WRITE_STATIC
				|| op->getOpId() == LOCK || op->getOpId() == POST  || op->getOpId() == UI_POST  || op->getOpId() == NATIVE_POST
				|| op->getOpId() == DELAY_POST){
			Debug(cerr<<"/*/*/*/*/*/*/*/*/ Potential dependent operation : "<<enumOpName[op->getOpId()]<<" /*/*/*/*/*/*/*/*/"<<endl);
			//dumpHBGraph();
			int dependentOpWithNoHB = partialProcessOperation(op,true,opProgramIndex);		// passing true for DPOR
			Debug(cerr<<" index of dependentOpWithNoHB = "<<dependentOpWithNoHB<<endl);
			if(dependentOpWithNoHB != -1){
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


void addBacktrackingChoicesEmdpor(int state, bool isFindTargetCallRecursive){
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
		if(op->getOpId() == READ || op->getOpId() == READ_STATIC || op->getOpId() == WRITE || op->getOpId() == WRITE_STATIC || op->getOpId() == LOCK){
			Debug(cerr<<"/*/*/*/*/*/*/*/*/ Potential dependent operation : "<<enumOpName[op->getOpId()]<<" /*/*/*/*/*/*/*/*/"<<endl);
			//			dumpHBGraph();
			int dependentOpWithNoHB = partialProcessOperation(op, false, opProgramIndex);			// passing false for emdpor
			Debug(cerr<<" index of dependentOpWithNoHB = "<<dependentOpWithNoHB<<endl);
			if(dependentOpWithNoHB != -1){
				findtarget_call_count++;
				pair<set<int>, int> res;
				if(!isFindTargetCallRecursive)
					res = emdpor_backtrack::computeBacktrack(dependentOpWithNoHB, op, false);
				else
					res = emdpor_backtrack::computeBacktrackRecursive(dependentOpWithNoHB, op, false, mapOfStateToAllTheSets);

				set<int> resultSet = res.first;
				if(! resultSet.empty()){
					Debug(cerr<<"-----Backtracking Set is not empty, Thread to run : " << *(resultSet.begin()) << ", BackTracking Point : " << res.second << endl);

					// add optimization here : try to add the thread which is already there in backtracking choice.
					addOptimalBacktrackingChoice(res.second, resultSet);

					// older one - adding without optimization.
//					addBacktrackingChoice(res.second, *(resultSet.begin()));
				}
				else{
					Debug(cerr<<"-----Backtracking Set is empty, BackTracking Point : " << res.second << endl);

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
		delete op;
	}
	Debug(cerr<<"addBacktrackingChoices() :: end"<<endl);
}

// this one should be called from backtrackEager(dpor_until_i, emdpor_until_j)
void addBacktrackingChoicesEmdpor_v2(int state, bool isFindTargetCallRecursive){
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
		if(op->getOpId() == READ || op->getOpId() == READ_STATIC || op->getOpId() == WRITE || op->getOpId() == WRITE_STATIC || op->getOpId() == LOCK){
			Debug(cerr<<"/*/*/*/*/*/*/*/*/ Potential dependent operation : "<<enumOpName[op->getOpId()]<<" /*/*/*/*/*/*/*/*/"<<endl);
			//			dumpHBGraph();
			int dependentOpWithNoHB = partialProcessOperation(op, false, opProgramIndex);			// passing false for emdpor
			Debug(cerr<<" index of dependentOpWithNoHB = "<<dependentOpWithNoHB<<endl);
			if(dependentOpWithNoHB != -1){
				findtarget_call_count++;
				pair<set<int>, int> res;
				if(!isFindTargetCallRecursive)
					res = emdpor_backtrack::computeBacktrack(dependentOpWithNoHB, op, false);
				else
					res = emdpor_backtrack::computeBacktrackRecursive(dependentOpWithNoHB, op, false, mapOfStateToAllTheSets);

				set<int> resultSet = res.first;
				if(! resultSet.empty()){
					Debug(cerr<<"-----Backtracking Set is not empty, Thread to run : " << *(resultSet.begin()) << ", BackTracking Point : " << res.second << endl);

					// add optimization here : try to add the thread which is already there in backtracking choice.
					addOptimalBacktrackingChoice(res.second, resultSet);

					// older one - adding without optimization.
//					addBacktrackingChoice(res.second, *(resultSet.begin()));
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
		delete op;
	}
	Debug(cerr<<"addBacktrackingChoices() :: end"<<endl);
}

//whenever this is called, it should return a value grater than -1.
int getMax_j_forWhichFindTargetFailed(){
	Debug(cerr<<"fn : getMax_j_forWhichFindTargetFailed() :: beginning"<<endl);
	int maxJ = -1;

//	for(set<int>::iterator it = findTargetFailed_state.begin(); it != findTargetFailed_state.end(); ++it){
//		Debug(cerr<<"state : "<<*it<<endl);
//	}

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
			Debug(cerr<<"									 ---| TRACE - 1 |----  "<<endl);
		}

		if(state>0){
			if(configId==DPOR_MODE){
				addBacktrackingChoicesDPOR(state);
			}
			else if(configId==EMDPOR_MODE){
				addBacktrackingChoicesEmdpor(state, true);
			}
		}
		if(op == NULL) {cerr<<"Error :: in fn:execute() getNextOperation returned NULL"<<endl; throw error;}
		Debug(cerr<<endl<<endl<<"--------------------------------------------------------------------------------------------State is : "<<state<<endl);
		Debug(op->dumpOpInfo());

		int programIndex = op->getOpIndex();
		addIntoSequence(programIndex, state+1);
		addBacktrackingChoice(state+1, op->getThreadId());
		addIntoDoneSet(state+1, op->getThreadId());
		op->setOpIndex(state+1);
		explored_transitions_count++;

		if(configId==DPOR_MODE)
			processOperation(op, true, false);
		else
			processOperation(op, false, false);
		explicitDependencies.resolveDependency(programIndex);
		addExplicitDependenciesState(state+1, explicitDependencies);
		addExplicitHBEdges(programIndex, op->getThreadId());
	}
	Debug(cerr<<"executeProgram() :: End ");
}

inline void resetDataStructuresBeforeReplay(){
//	Debug(dumpHBGraph());
	eraseBookKeeper();
	eraseClock();
	pruneSequence();
	resetExplicitDependencies();
	findTargetFailed_state.erase(findTargetFailed_state.begin(), findTargetFailed_state.end());
//	Debug(dumpHBGraph());
}

inline void resetDataStructuresBeforeReplay(int pruneIndex){
//	Debug(dumpHBGraph());
	eraseBookKeeper();
	eraseClock();
//	pruneSequence(pruneIndex);						// here is the only change in this overload
	resetExplicitDependencies();
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
		addExplicitHBEdges(programIndex, op->getThreadId());
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


	if(configId==EMDPOR_MODE){
		reportfile<<"#findTarget failed \t\t\t"<< findtarget_fail_count << endl;
		reportfile<<"#eagerly-replayed transitions \t\t"<< eagerReplay_transition_count << endl;
	}

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
		Debug(cerr<<"case 2"<<endl);
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
			Debug(cerr<<"									 ---| TRACE - 1 |----  "<<endl);
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
			addBacktrackingChoicesEmdpor(state, true);
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
				Debug(cerr<<"case 2"<<endl);
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
			cerr<<"Only 1 argument expected: path to the configuration file config.cfg"<<endl;
			throw error;
		}

		if(!preProcess(argv[1])){
			cerr<<"main() : ERROR :: preProcess() failed!!"<<endl;
			throw error;
		}
		Debug(cerr<<"programLength : "<<programLength<<endl);
		initClock(programLength, emdpor_preprocessor_private::getMaxThreadId());

		// setting configuration to run only emdpor in this repo branch
		configId=DPOR_MODE;

		if(configId==DPOR_MODE){
			reportProgramDetails();
			explore(DPOR_MODE);
			writeReport(tStart,true);
			cerr<<endl<<endl<<"\t\t---- DPOR ----"<<endl<<endl;
			cerr<<"#findTarget called \t\t\t"<< findtarget_call_count << endl;

			//(replays not included)
			cerr<<"#explored transitions \t\t\t"<< explored_transitions_count << endl;

			cerr<<"#replayed transitions \t\t\t"<< replay_transition_count << endl;
			cerr<<"#traces explored \t\t\t"<< trace_count << endl;
			cerr<<"Time taken \t\t\t\t"<< (double)(clock() - tStart)/CLOCKS_PER_SEC<<endl;
		}
		else if(configId==EMDPOR_MODE){
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
