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
#include <VectorClock.h>
#include <Sets.h>
#include <Util.h>

using namespace std;
using namespace emdpor_bookKeeper;
using namespace emdpor_Entities;
using namespace dpor_vc;

namespace emdpor_backtrack{

inline int getState(int traceIndex){return (traceIndex-1);}

inline Operation * post(int taskId){
	if(taskId==-1){ cerr<<"backtrack::fn:task(), input operation was not executed inside a task!!"<<endl; throw error;}
	return getOperation(getParentPostIndex(taskId));
}


/*
 * returns a pair of 1) vector of threadIds. 2) index in trace : backtracking point.
 */
pair< set<int>, int> computeBacktrack(int i, Operation* opJ, bool isRecursiveCall){		// should satisfy i<j
	//	Debug(cerr<<"fn:computeBacktrack() : begin "<<endl);

	set<int> resultSet;
	if(i==-1 || opJ == NULL){
		cerr<<"Error : in fn:computeBacktrack, invalid arguments passed!"<<endl;
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

	i = op1->getOpIndex();
	int threadIdOfI = op1->getThreadId();
//	Debug(cerr<<endl<<endl<<" state :  "<<getState(i)<<endl);
//	Debug(cerr<<" Thread ID2 :  "<< op2->getThreadId() <<", Thread ID1 : "<< op1->getThreadId() <<endl);


	Thread threadState = getThreadState(getState(i), op2->getThreadId());


	if(isThreadEnabled(getState(i), op2->getThreadId(), threadState) && isTaskExecutable(op2->getTaskId(), threadState)){
		Debug(cerr<<"Inserting thread id : "<<op2->getThreadId()<<endl);
		resultSet.insert(op2->getThreadId());
	}

	set<int> happenedBeforeThreadIds = findTaskExecutableHappenedBeforeThread(getState(i), op2->getOpIndex(), op2->getThreadId(), threadIdOfI);

	for(set<int>::iterator sit = happenedBeforeThreadIds.begin(); sit != happenedBeforeThreadIds.end(); ++sit)
		resultSet.insert(*sit);

	return make_pair(resultSet,i);
}


pair< set<int>, int> computeBacktrackRecursive(int i, Operation* opJ, bool isRecursiveCall, const map<int, Sets> &mapOfStateToAllTheSets){		// should satisfy i<j
	//	Debug(cerr<<"fn:computeBacktrack() : begin "<<endl);
    set<int> resultSet;
	if(i==-1 || opJ == NULL){
		cerr<<"Error : in fn:computeBacktrackRecursive, invalid arguments passed!"<<endl;
		throw error;
		return make_pair(resultSet,i);
	}
	Operation * op1 = getOperation(i);
	Operation * op2 = opJ;

	while(op1->getThreadId() == op2->getThreadId()){			//means they are running on same thread : i.e. a thread with Q.
		int taskId1, taskId2;
		taskId1 =  op1->getTaskId();
		taskId2 =  op2->getTaskId();
		if(taskId1 == -1 || taskId2 == -1)						// reaching a thread without queue also breaks the loop.
			break;
		op1 = post(taskId1);
		op2 = post(taskId2);
	}

	i = op1->getOpIndex();
	int threadIdOfI = op1->getThreadId();
//	Debug(cerr<<endl<<endl<<" state :  "<<getState(i)<<endl);
//	Debug(cerr<<" Thread ID2 :  "<< op2->getThreadId() <<", Task Id2 : "<< op2->getTaskId() <<endl);

	Thread threadState = getThreadState(getState(i), op2->getThreadId());

	if(isThreadEnabled(getState(i), op2->getThreadId(), threadState)/* && isTaskExecutable(op2->getTaskId(), threadState) */ ){
		Debug(cerr<<"Inserting thread id : "<<op2->getThreadId()<<endl);
		resultSet.insert(op2->getThreadId());
	}


	set<int> happenedBeforeThreadIds = findHappenedBeforeEnabledThreads(getState(i), op2->getOpIndex(), threadIdOfI);
	for(set<int>::iterator sit = happenedBeforeThreadIds.begin(); sit != happenedBeforeThreadIds.end(); ++sit){
		Debug(cerr<<"Inserting thread id : "<<*sit<<endl);
		resultSet.insert(*sit);
	}

	set<int> doneSet_s;
    map<int, Sets>::const_iterator mapIter = mapOfStateToAllTheSets.find(i);
	if(mapIter != mapOfStateToAllTheSets.end()){
		doneSet_s = mapIter->second.getImmutableDoneSet();
	}

	Debug(cerr<<"Done set at state "<<i<<" "<<endl);
	Debug(dumpSet(doneSet_s));
	Debug(cerr<<"Result set "<<endl);
	Debug(dumpSet(resultSet));
	set<int> resSetPrime = setDiff(resultSet, doneSet_s);
	Debug(cerr<<"Result set "<<endl);
	Debug(dumpSet(resultSet));
	Debug(cerr<<"Result set prime "<<endl);
	Debug(dumpSet(resSetPrime));

	if(!resSetPrime.empty())
		return make_pair(resSetPrime,i);


	map<int, set<int> > HBEnqueuedTasks = findHappenedBeforeEnqueuedTasks(getState(i), op2->getOpIndex());
	if(!HBEnqueuedTasks.empty()){
		map<int, set<int> >::iterator it = HBEnqueuedTasks.begin();
		int luckyThreadId = it->first;
		Thread luckyThread = getThreadState(getState(i), luckyThreadId);
		if(!luckyThread.isNullThread()){
			int nearestTask = luckyThread.getTaskNearestFoQ(it->second);
			int executableTask = luckyThread.getExecutableTask();
			Debug(cerr<<" Calling computeBacktrack( "<< getParentPostIndex(executableTask) << ", " << getParentPostIndex(nearestTask) << " ) "<<endl);
			return computeBacktrackRecursive(getParentPostIndex(executableTask), getOperation(getParentPostIndex(nearestTask)), true, mapOfStateToAllTheSets);
		}
	}

	return make_pair(resultSet,i);
}

}

#endif /* COMPUTEBACKTRACK_H_ */
