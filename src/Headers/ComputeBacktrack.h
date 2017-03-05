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
 * This file contains the implementation of FindTarget and ReschedulePending
 * algorithm presented in the paper.
 */

#ifndef COMPUTEBACKTRACK_H_
#define COMPUTEBACKTRACK_H_

#include <stddef.h>
#include <iostream>
#include <map>
#include <set>
#include <utility>

#include <BookKeeping.h>
#include <Debug.h>
#include <Entities.h>
#include <EmdporVectorClock.h>
#include <Sets.h>
#include <Util.h>

using namespace std;
using namespace emdpor_bookKeeper;
using namespace emdpor_Entities;
using namespace emdpor_vc;

namespace emdpor_backtrack{

inline int getState(int traceIndex){return (traceIndex-1);}

inline Operation * post(int taskId){
	if(taskId==-1){ cerr<<"backtrack::fn:task(), input operation was not executed inside a task!!"<<endl; throw error;}
	return getOperation(getParentPostIndex(taskId));
}

//forward declarations
set< pair< set<int>, int> > findTarget(int i, Operation* opJ, map<int, Sets> &mapOfStateToAllTheSets);

static bool firstCall = true;		// needed for check of line-3 in new algo.

int reschedulePendingWorklistCounter = 0;
/*
 * returns a pair of 1) vector of threadIds. 2) index in trace : backtracking point.
 */
pair< set<int>, int> computeBacktrack(int i, Operation* opJ, bool isRecursiveCall){		// should satisfy i<j
	set<int> resultSet;
	if(i==-1 || opJ == NULL){
		cerr<<"Error : in fn:computeBacktrackEmdpor, invalid arguments passed!"<<endl;
		throw error;
		return make_pair(resultSet,i);
	}
	Operation * op1 = getOperation(i);
	Operation * op2 = opJ;


	while(op1->getThreadId() == op2->getThreadId()){			//means they are running on same thread : i.e. a thread with Q.
//		Debug(cerr<<" op index1 :  "<< op1->getOpIndex() <<", op index2 : "<< op2->getOpIndex() <<endl);
		int taskId1, taskId2;
		taskId1 =  op1->getTaskId();
		taskId2 =  op2->getTaskId();
//		Debug(cerr<<" task id 1 :  "<< taskId1 <<", task id 2 : "<< taskId2 <<endl);
		if(taskId1 == -1 || taskId2 == -1)						// reaching a thread without queue also breaks the loop.
			break;
		op1 = post(taskId1);
		op2 = post(taskId2);
	}

	// op2 != opJ means lock-step progressed and hence op2 is a post operation.
//	if(!isRecursiveCall && op2!=opJ)
//		HBGraphUtility::addPostDependenceHook(op1->getOpIndex(), op2->getOpIndex());

	i = op1->getOpIndex();
	int threadIdOfI = op1->getThreadId();
//	Debug(cerr<<endl<<endl<<" state :  "<<getState(i)<<endl);
//	Debug(cerr<<" Thread ID2 :  "<< op2->getThreadId() <<", Thread ID1 : "<< op1->getThreadId() <<endl);


	Thread threadState = getThreadState(getState(i), op2->getThreadId());

//	threadState.dumpThread();

	if(isThreadEnabled(getState(i), op2->getThreadId(), threadState) && isTaskExecutable(op2->getTaskId(), threadState)){
		Debug(cerr<<"Inserting thread id : "<<op2->getThreadId()<<endl);
		resultSet.insert(op2->getThreadId());
	}

	set<int> happenedBeforeThreadIds = findTaskExecutableHappenedBeforeThread(getState(i), op2, threadIdOfI, false);
	for(set<int>::iterator sit = happenedBeforeThreadIds.begin(); sit != happenedBeforeThreadIds.end(); ++sit)
		resultSet.insert(*sit);

	return make_pair(resultSet,i);
}


int getExecutableTaskId(int state, int threadId){
	Debug(cerr<<"Begin :: fn-getExecutableTaskId()"<<endl);
	Thread thread = getThreadState(state, threadId);
	if(!thread.isNullThread()){
		Debug(cerr<<"End :: fn-getExecutableTaskId()"<<endl);
		return thread.getExecutableTask();
	}

	cerr<<"ERROR::Couldn't get thread for  thread id = "<<threadId<<endl;
	throw error;

}

set< pair< set<int>, int> > reschedulePending(int i, map<int, set<int> > pendingTasks, map<int, Sets> &mapOfStateToAllTheSets){
	Debug(cerr<<"fn:reschedulePending() : begin "<<endl);
	Debug(cerr<<"i:"<<i<<"all pending tasks grouped by thread ids - "<<endl);
	Debug(dumpIntMapToSet(pendingTasks));
	map<int, set<int> > swapMap;
	set<int> workList;
	map<int, set<int> >::iterator it = pendingTasks.begin();
	int luckyThreadId = it->first;
	int executableTaskId = getExecutableTaskId(getState(i), luckyThreadId);
	Debug(cerr<<"chosen thread: "<<luckyThreadId<<", executable task id of this thread: "<<executableTaskId<<endl);
	set<int> luckyThreadPendingEvents = it->second;
	Debug(cerr<<"dumping pending events of chosen thread - "<<endl);
	Debug(dumpSet(luckyThreadPendingEvents));

	if(executableTaskId != -1 && !luckyThreadPendingEvents.empty()){
		workList.insert(executableTaskId);
		swapMap[luckyThreadId].insert(*(luckyThreadPendingEvents.begin()));
		Debug(cerr<<"dumping swapMap"<<endl);
		dumpIntMapToSet(swapMap);
		Debug(cerr<<"dumping workList"<<endl);
		dumpSet(workList);
	}
	else{
		cerr<<"ERROR::executable task Id is -1, for thread id: "<<luckyThreadId<<" in state: <<i<<endl";
		throw error;
	}

	while(! workList.empty() ){

		int taskId = *(workList.begin());
		workList.erase(workList.begin());
		PostOperation * pop = dynamic_cast<PostOperation *>(getOperation(getParentPostIndex(taskId)));
		/* unlike the ReschedulePending algorithm given in the paper, we do not check for
		 * wavy HB defined in the paper between operation at index i and executable task of a thread.
		 * Thus we compute a superset of blocked - enabled pairs to be reordered than what algorithm in
		 * the paper requires.
		 */
		map<int, set<int> > C = findHappenedBeforeEnqueuedTasks(getState(i), getOperation(pop->getEndOpIndex()), false);

		Debug(if (C.empty()) cerr<<"C is empty!"<<endl);

		for(map<int, set<int> >::iterator it = C.begin(); it != C.end(); ++it){
			Debug(cerr<<"working on C"<<endl);
			int threadId = it->first;
			set<int> blockedEvents = it->second;
			executableTaskId = getExecutableTaskId(getState(i), threadId);
			if(executableTaskId != -1 && !blockedEvents.empty()){
				workList.insert(executableTaskId);

				reschedulePendingWorklistCounter++;
				Debug(cerr<<"adding executableTaskId: "<<executableTaskId<<" to worklist! current counter value: "<<reschedulePendingWorklistCounter<<endl);

				swapMap[threadId].insert(*(blockedEvents.begin()));
				Debug(cerr<<"dumping swapMap"<<endl);
				dumpIntMapToSet(swapMap);
			}else{
				cerr<<"ERROR::executable task Id is -1, for thread id: "<<luckyThreadId<<" in state: <<i<<endl";
				throw error;
			}
		}
	}

	Debug(cerr<<"worklist is empty, starting recursive call loop"<<endl);

	set< pair< set<int>, int> > findTargetResults;

	for(map<int, set<int> >::iterator it = swapMap.begin(); it != swapMap.end(); ++it){
		Debug(cerr<<"inside the loop"<<endl);
		int threadId = it->first;
		set<int> blockedEvents = it->second;
		int lastPostedEvent;
		Debug(cerr<<"threadId: "<<threadId<<endl);

		if(!blockedEvents.empty()){
			lastPostedEvent = *(blockedEvents.rbegin());
		}else{
			cerr<<"ERROR::set of events - swapMap["<<threadId<<"] is empty!"<<endl;
			throw error;
		}
		Debug(cerr<<"lastPostedEvent = "<<lastPostedEvent<<endl);
		executableTaskId = getExecutableTaskId(getState(i), threadId);
		Debug(cerr<<"executable task id of above thread id: "<<executableTaskId<<endl);
		Debug(cerr<<" calling findTarget( "<< getParentPostIndex(executableTaskId) << ", " << getParentPostIndex(lastPostedEvent) << " ) "<<endl);
		firstCall = false;
		set< pair< set<int>, int> > result = findTarget(getParentPostIndex(executableTaskId), getOperation(getParentPostIndex(lastPostedEvent)), mapOfStateToAllTheSets);
		for(set< pair< set<int>, int> >::iterator complexItr = result.begin(); complexItr != result.end(); ++complexItr){
			findTargetResults.insert(*complexItr);
		}
	}
	Debug(cerr<<"fn:reschedulePending() : end "<<endl);
	return findTargetResults;
}

/* Implementation of FindTarget algorithm given in the paper. This identifies backtracking points
 * and backtracking choices to reorder opJ with already executed dependent operation at index i
 * in the sequence being currently explored.
 */
set< pair< set<int>, int> > findTarget(int i, Operation* opJ, map<int, Sets> &mapOfStateToAllTheSets){		// should satisfy i<j
	Debug(cerr<<"fn:findTarget() : begin "<<endl);
	set< pair< set<int>, int> > returnValue;
	set<int> resultSet;
	if(i==-1 || opJ == NULL){
		cerr<<"ERROR : in fn:computeBacktrackRecursive, invalid arguments passed!"<<endl;
		throw error;
		returnValue.insert(make_pair(resultSet,i));
		return returnValue;
	}
	Operation * op1 = getOperation(i);
	Operation * op2 = opJ;

	if(!firstCall){
		if(if_i_HB_task(op1,op2,false)){
			resultSet.insert(op1->getThreadId());  //since op1 HB op2, trivially return the already executed thread of op1
			firstCall = true;					   // setting it true for further non-recursive calls to findTarget.
			returnValue.insert(make_pair(resultSet,op1->getOpIndex()));
			return returnValue;  	               // equivalent to empty return without invoking backtrack eager.
		}
	}

	//find diverging posts
	while(op1->getThreadId() == op2->getThreadId()){			//means they are running on same thread : i.e. a thread with Q.
		int taskId1, taskId2;
		taskId1 =  op1->getTaskId();
		taskId2 =  op2->getTaskId();
		if(taskId1 == -1 || taskId2 == -1)						// reaching a thread without queue also breaks the loop.
			break;
		op1 = post(taskId1);
		op2 = post(taskId2);

		if(if_i_HB_task(op1,op2,false)){
			resultSet.insert(op1->getThreadId());
			firstCall = true;						// setting it true for further non-recursive calls to findTarget.
			returnValue.insert(make_pair(resultSet,op1->getOpIndex()));
			return returnValue;  	                // equivalent to empty return without invoking backtrack eager.
		}
	}

	//re-assign i since computation of diverging posts may have reassigned op1. Var i should correspond to index of op1 in current sequence.
	i = op1->getOpIndex();
	int threadIdOfI = op1->getThreadId();
	Debug(cerr<<endl<<endl<<" state :  "<<getState(i)<<endl);
	Debug(cerr<<" Thread ID1 :  "<< op1->getThreadId() <<", Task Id1 : "<< op1->getTaskId() <<endl);
	Debug(cerr<<" Thread ID2 :  "<< op2->getThreadId() <<", Task Id2 : "<< op2->getTaskId() <<endl);

	//state of op2's thread at program state prior to execution of op1
	Thread threadState = getThreadState(getState(i), op2->getThreadId());


	if(isThreadEnabled(getState(i), op2->getThreadId(), threadState)/* && isTaskExecutable(op2->getTaskId(), threadState) */ ){
		Debug(cerr<<"Inserting thread id : "<<op2->getThreadId()<<endl);
		resultSet.insert(op2->getThreadId());
	}

	set<int> happenedBeforeThreadIds = findHappenedBeforeEnabledThreads(getState(i), op2, threadIdOfI, false);

	for(set<int>::iterator sit = happenedBeforeThreadIds.begin(); sit != happenedBeforeThreadIds.end(); ++sit){
		Debug(cerr<<"Inserting thread id : "<<*sit<<endl);
		resultSet.insert(*sit);
	}

	set<int> doneSet_s;
	map<int, Sets>::const_iterator mapIter = mapOfStateToAllTheSets.find(i);
	if(mapIter != mapOfStateToAllTheSets.end()){
		doneSet_s = mapIter->second.getImmutableDoneSet();
	}

	Debug(cerr<<"Result set "<<endl);
	Debug(dumpSet(resultSet));
	Debug(cerr<<"Done set at state "<<i<<" "<<endl);
	Debug(dumpSet(doneSet_s));
	set<int> resSetPrime = setDiff(resultSet, doneSet_s);
	Debug(cerr<<"Result set prime "<<endl);
	Debug(dumpSet(resSetPrime));

	if(!resSetPrime.empty()){
		firstCall = true;									// setting it true for further fresh calls.
		if(isPostOperation(op1) && isPostOperation(op2)){
			map<int, Sets>::iterator mapIter = mapOfStateToAllTheSets.find(i);
			if(mapIter != mapOfStateToAllTheSets.end()){
				/* indicate that op2 was identified to be reordered w.r.t. post op1 executed in state s_i.
				 * This info will be used later to populate reorderedPosts field of Explore.cpp before
				 * exploring a new sequence, which in turn will be used to add HB edges from op2 to op1
				 * when they actually get reordered in a later explored  sequence.
				 */
				mapIter->second.addReorderedSets(mapContolLocationToProgramIndex(op2->getOpIndex()), mapContolLocationToProgramIndex(op1->getOpIndex()));
			}
		}
		returnValue.insert(make_pair(resSetPrime,i));
		return returnValue;
	}

	map<int, set<int> > HBEnqueuedTasks = findHappenedBeforeEnqueuedTasks(getState(i), op2, false);
	if(!HBEnqueuedTasks.empty()){
		return reschedulePending(i, HBEnqueuedTasks, mapOfStateToAllTheSets);
	}

	firstCall = true;
	if(isPostOperation(op1) && isPostOperation(op2)){
		map<int, Sets>::iterator mapIter = mapOfStateToAllTheSets.find(i);
		if(mapIter != mapOfStateToAllTheSets.end()){
			mapIter->second.addReorderedSets(mapContolLocationToProgramIndex(op2->getOpIndex()), mapContolLocationToProgramIndex(op1->getOpIndex()));
		}
	}
	returnValue.insert(make_pair(resultSet,i));
	return returnValue;
}

}


#endif /* COMPUTEBACKTRACK_H_ */
