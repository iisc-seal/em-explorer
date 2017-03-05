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


#ifndef BOOKKEEPING_H_
#define BOOKKEEPING_H_

#include <utility>
#include <map>
#include <vector>


#include <Preprocessor.h>

using namespace std;
using namespace emdpor_Entities;
using namespace emdpor_preprocessor;

namespace emdpor_bookKeeper{

/******* a few forward declarations *******/

void saveState(int index, map<int, Thread> state);
void updateThreadQ(int index, int threadId, int taskId);
int getParentForkIndex(int threadId);

Thread useLess;
Thread & nullThread = useLess;



/******* Compare classes *******/

struct compareLockStateKey {						//comparison class for below key:returns true if lhs should come before rhs!
	bool operator() (const pair<int, long>& lhs, const pair<int, long>& rhs) const
	{
		if(lhs.first < rhs.first)
			return true;
		else if((lhs.first == rhs.first) && (lhs.second < rhs.second))
			return true;
		else
			return false;
	}
};

struct compareEnableKey {						//comparison class for above key:returns true if lhs should come before rhs!
	bool operator() (const pair<long, string>& lhs, const pair<long, string>& rhs) const
	{
		if(lhs.first < rhs.first)
			return true;
		else if((lhs.first == rhs.first) && lhs.second.compare(rhs.second)<0)
			return true;
		else
			return false;
	}
};

struct compareReadWrtieKey {						//comparison class for above key:returns true if lhs should come before rhs!
	bool operator() (const pair<long, int>& lhs, const pair<long, int>& rhs) const
	{
		if(lhs.first < rhs.first)
			return true;
		else if((lhs.first == rhs.first) && (lhs.second < rhs.second))
			return true;
		else
			return false;
	}
};

struct compareStaticAccessKey {						//comparison class for above key:returns true if lhs should come before rhs!
	bool operator() (const pair<string, int>& lhs, const pair<string, int>& rhs) const
	{
		if(lhs.first.compare(rhs.first)<0)
			return true;
		else if((lhs.first.compare(rhs.first)==0) && lhs.second<rhs.second)
			return true;
		else
			return false;
	}
};


/******* Data Structures *******/

map< pair<int, long>, int > lockState;		// key: <index, lock-object id>, value : threadId which holds this lock at this index(state)
map<int, vector<int> > threadIndicesMap;	// key - threadId, values - vector of operation indices executed on that thread.
map<int, Thread> threadMap;					// Set of threads which have executed threadinit but not yet executed threadexit
map<long, int> locker;						//key: locked object id, Value : thread that took the lock
map<int, Operation*> traceMap;			int traceLength=0;				// Book-keeping for Trace
map<int, map<int, Thread> > stateMap;		//key: Index in Trace, Value: map<threadId, thread Object>
map<int, ExplicitDependencies> explicitDependenciesStateMap;			//key: Index(control location) in trace, value : ExplicitDependencies object as state of these dependencies.
map<int,int>forkMap;						//Not saving fork info in threadMap until I see a threadInit to be able to answer backtracking questions!
map<int,int>postMap;						//1. postedMessageId, 2. postOperationIndex
map<int,int>postHookMap;					//1. postedMessageId, 2. postOperationIndex
map < int, vector < pair<int, int> > > threadPostMap;					// key - TargetThreadId, value - pair ( postIndex, threadFromWherePost was made)
map< pair<long, string>, int, compareEnableKey >enableMap;				//key: pair of 1. id, 2. state, value: index of Enable Operation
map< pair<long,int>, vector<int>, compareReadWrtieKey > heapReadMap;	// key : < object id, offset>, Value: vector<heapAccessIndices>
map< pair<long,int>, vector<int>, compareReadWrtieKey > heapWriteMap;	// key : < object id, offset>, Value: vector<heapAccessIndices>
map< pair<string,int>, vector<int>, compareStaticAccessKey > stackReadMap;	//key : <className, offset>, Value: vector<memAccessIndices>
map< pair<string,int>, vector<int>, compareStaticAccessKey > stackWriteMap;
map<int,int> taskLastOpMap;					// 1. int taskId 2. int lastOpIndex. ||| remove entry after seeing END operation
map<int,int> threadLastOpMap;				// 1. int threadId 2. int lastOpIndex. ||| remove entry after seeing END operation
map<int,int> attachQIndexMap;				// 1. Thread ID, 2. attachQ operation index for this thread ID.
map<long,int> lockUnlockMap;				// key : object id, Value: Index of last lock or unlock operation for particular lock object.
map<int,int> loopIndexMap;					// 1. Thread ID, 2. attachQ operation index for this thread ID.
map<int,vector<int> > notifyIndexMap;		// key : NotifiedThreadId, Value: vector of Indices of Notify Operations
map<long, vector<int> > lockMap;			// key: lock obj id, Value: vector<lock op index with same key>-	Used to store all locations where lock was taken!
map< long, vector<int>  > unlockMap;		// key - objectId, value - index




/****** lockMap auxiliary functions *******/

void addLock(long id, int index){
	map<long, vector<int> >::iterator it = lockMap.find(id);
	vector<int> lockIndices;
	if(it != lockMap.end())
		lockIndices = it->second;
	lockIndices.push_back(index);
	lockMap[id] = lockIndices;
}

vector<int> getLocksOnId(long id){
	map<long, vector<int> >::iterator it = lockMap.find(id);
	vector<int> lockIndices;
	if(it != lockMap.end())
		lockIndices = it->second;
	return lockIndices;
}

/****** eraser function *******/
void eraseBookKeeper(){
	lockState.erase(lockState.begin(), lockState.end());
	threadIndicesMap.erase(threadIndicesMap.begin(), threadIndicesMap.end());
	threadMap.erase(threadMap.begin(), threadMap.end());
	locker.erase(locker.begin(), locker.end());
	stateMap.erase(stateMap.begin(), stateMap.end());
	forkMap.erase(forkMap.begin(), forkMap.end());
	postMap.erase(postMap.begin(), postMap.end());
	enableMap.erase(enableMap.begin(), enableMap.end());
	heapReadMap.erase(heapReadMap.begin(), heapReadMap.end());
	heapWriteMap.erase(heapWriteMap.begin(), heapWriteMap.end());
	stackReadMap.erase(stackReadMap.begin(), stackReadMap.end());
	stackWriteMap.erase(stackWriteMap.begin(), stackWriteMap.end());
	taskLastOpMap.erase(taskLastOpMap.begin(), taskLastOpMap.end());
	threadLastOpMap.erase(threadLastOpMap.begin(), threadLastOpMap.end());
	attachQIndexMap.erase(attachQIndexMap.begin(), attachQIndexMap.end());
	lockUnlockMap.erase(lockUnlockMap.begin(), lockUnlockMap.end());
	loopIndexMap.erase(loopIndexMap.begin(), loopIndexMap.end());
	notifyIndexMap.erase(notifyIndexMap.begin(), notifyIndexMap.end());
	lockMap.erase(lockMap.begin(), lockMap.end());
	explicitDependenciesStateMap.erase(explicitDependenciesStateMap.begin(), explicitDependenciesStateMap.end());
	postHookMap.erase(postHookMap.begin(), postHookMap.end());
	threadPostMap.erase(threadPostMap.begin(), threadPostMap.end());
	unlockMap.erase(unlockMap.begin(), unlockMap.end());

	while(! traceMap.empty()){
		Operation * op = traceMap.begin()->second;
		traceMap.erase(traceMap.begin());
		delete op;
	}

	traceLength=0;
}


/****** Book-Keeping for WAIT/NOTIFY Operations *******/

void addNotifyOperation(int notifiedThreadId, int index){
	map<int, vector<int> >::iterator it = notifyIndexMap.find(notifiedThreadId);
	vector<int> notifyOps;
	if(it!=notifyIndexMap.end())
		notifyOps = it->second;
	notifyOps.push_back(index);
	notifyIndexMap[notifiedThreadId] = notifyOps;
}

int popNotifyOperation(int threadId){
	map<int, vector<int> >::iterator it = notifyIndexMap.find(threadId);
	if(it==notifyIndexMap.end())
		return -1;
	vector<int> notifyOps = it->second;
	if(notifyOps.empty())
		return -1;
	int index = *(notifyOps.begin());
	notifyOps.erase(notifyOps.begin());
	notifyIndexMap[threadId] = notifyOps;
	return index;
}

bool isWaitNotified(int threadId){
	map<int, vector<int> >::iterator it = notifyIndexMap.find(threadId);
	if(it==notifyIndexMap.end())
		return false;
	vector<int> notifyOps = it->second;
	if(notifyOps.empty())
		return false;
	return true;
}


/****** Book-Keeping for maintaining Lock states. *******/

void addLockState(pair<int, long> key, int threadId){			//Key consists of index(state) and lock object Id
	map < pair<int, long>, int >::iterator it = lockState.find(key);
	if(it!=lockState.end()){
		cerr<<"ERROR!! Trying to take Lock on already Locked Object. \n\t previous index : "<<it->first.first<<" Locked Object Id : "\
				<<it->first.second<<" threadId that took lock : "<<it->second<<" current Index : "<<key.first<<endl;
		throw error;
		return;
	}
	lockState[key] = threadId;
}

void dumpLockState(){
	map < pair<int, long>, int > :: iterator it = lockState.begin();
	for( ; it != lockState.end(); ++it){
		cerr<<" Index : "<<it->first.first<<" Lock Object ID : "<<it->first.second<<" Holder Thread Id : "<<it->second<<endl;
	}
}

int getLockHolderThreadId(pair<int, long> key){
	//	Debug(dumpLockState());
	map < pair<int, long>, int >::iterator it = lockState.find(key);
	if(it!=lockState.end())
		return it->second;
	return -1;
}

/****** Book-keeping for Threads *******/

void addInstruction(int threadId, int index){
	map<int, vector<int> >::iterator it = threadIndicesMap.find(threadId);
	vector<int> threadIndices;
	if(it != threadIndicesMap.end()){
		threadIndices = it->second;
	}
	threadIndices.push_back(index);
	threadIndicesMap[threadId] = threadIndices;
}


/*
 * Here what you get (return value) is Next instruction index in the program that should be executed on given threadId
 * NOTE that it is NOT a CONTROL LOCATION and it should be mapped to CL before execution.
 */
int getNextInstructionForExecution(Thread threadState, int afterIndex){		//returns next instruction index of a thread which has not been executed yet
	int result = -1;
	if(threadState.isLooping()){									// Thread is looping
		if(threadState.getTaskId() != -1){							// Thread is running some task
			result = getNextTaskInstruction(threadState.getTaskId(), afterIndex);
			return result;
		}
		else if(threadState.getFrontOfQueue() != -1){				// Thread is not running any task but its queue is not empty
			result = getNextTaskInstruction(threadState.getFrontOfQueue(), -1);		//get Begin op of front task in queue
			return result;
		}
		else							// Thread has empty queue with no task in execution
			return -1;
	}
	else{
		if(getParentForkIndex(threadState.getThreadId()) != -1)				//Thread has been forked but not yet initialized!
			return getNextThreadInstruction(threadState.getThreadId(), -1);
		return getNextThreadInstruction(threadState.getThreadId(), afterIndex);
	}
}


/*
 * Here what you get (return value) is Next control location of a thread in some state (afterIndex)
 */
int getNextInstruction(int threadId, int state){		//returns next instruction index of a thread in some state(afterIndex)
	map<int, vector<int> >::iterator it = threadIndicesMap.find(threadId);
	if(it == threadIndicesMap.end())
		return -1;
	vector<int> threadIndices = it->second;
	int lastThreadIndex = *(threadIndices.rbegin());
	DebugSelective(cerr<<"fn:getNextInstruction() :: lastThreadIndex : "<<lastThreadIndex<<endl);
	int afterIndex = state + 1;
	while(afterIndex <= lastThreadIndex){
		/* the next instruction on threadId at the given state is the immediate operation executed
		 * by threadId. This can be identified by returning the first index between state+1 and
		 * lastThreadIndex which was assigned to an operation on this thread.
		 */
		vector<int>::iterator vit = find(threadIndices.begin(), threadIndices.end(), afterIndex);
		if(vit != threadIndices.end())
			return *vit;
		afterIndex++;
	}
	return -1;
}

// returns instruction of a thread on or before input state. threadState should not be null.
int getPreviousInstruction(int threadId, int state, Thread threadState){
	if(threadState.isNullThread()){
		cerr<<"ERROR :: fn:getPreviousInstruction() called with null threadState"<<endl;
		throw error;
	}
	map<int, vector<int> >::iterator it = threadIndicesMap.find(threadId);
	if(it == threadIndicesMap.end())
		return -1;
	vector<int> threadIndices = it->second;
	int firstThreadIndex = *(threadIndices.begin());

	while(firstThreadIndex <= state){
		vector<int>::iterator vit = find(threadIndices.begin(), threadIndices.end(), state);
		if(vit != threadIndices.end())
			return *vit;
		state--;
	}
	return -1;
}


void addThread(int threadId){
	map<int, Thread>::iterator it = threadMap.find(threadId);
	if(it == threadMap.end()){
		Thread t;
		t.setThreadId(threadId);
		threadMap[threadId] = t;
	}
}

void addThread(int threadId, int parentThreadId){
	map<int, Thread>::iterator it = threadMap.find(threadId);
	if(it == threadMap.end()){
		Thread t;
		t.setThreadId(threadId);
		t.setParentThreadId(parentThreadId);
		threadMap[threadId] = t;
	}
}

Thread & getThread(int threadId){
	map<int, Thread>::iterator it = threadMap.find(threadId);
	if(it == threadMap.end())
		return nullThread;
	Thread &t = it->second;
	return t;
}

void removeThread(int threadId){
	threadMap.erase(threadId);
}

int getTaskId(int threadId){
	Thread t = getThread(threadId);
	if(t.isNullThread())
		return -1;
	return t.getTaskId();
}

void updateQueue(int threadId, int taskId){
	Thread &t = getThread(threadId);
	if(t.isNullThread())
		return;
	t.insertIntoQ(taskId);
	//	t.dumpThread();
}


void incrementNotifyCount(int threadId){
	Thread &t = getThread(threadId);
	t.incrementNotifyCount();
}

void decrementNotifyCount(int threadId){
	Thread &t = getThread(threadId);
	t.decrementNotifyCount();
}

/******* Hack for adding lock states *******/

void startLocker(long objectId, int lockHolderThreadId){
	locker[objectId] = lockHolderThreadId;
}

bool isSomeLockActive(){
	return !locker.empty();
}

void stopLocker(long objectId){
	locker.erase(objectId);
}


/******* Book-keeping for Trace *******/

void addOperation(int index, Operation *op){
	traceMap[index]=op;
	traceLength++;
	map<int, Thread> aState = map<int, Thread>(threadMap);
	saveState(index, aState);
	addInstruction(op->getThreadId(), index);
	if(isSomeLockActive()){
		for(map<long,int>::iterator it = locker.begin(); it != locker.end(); ++it){
			addLockState(make_pair(index,it->first), it->second);
		}
	}
}

void addLockOperation(int index, Operation *op, long objectId, int lockHolderThreadId){
	startLocker(objectId, lockHolderThreadId);
	addOperation(index, op);
}

void addUnlockOperation(int index, Operation *op, long objectId){
	stopLocker(objectId);
	addOperation(index, op);
}

void addPostOperation(int index, Operation *op){
	Debug(if(op==NULL) cerr<<"Attempt to store a Null operation in traceMap, BookKeeping.h:addOperation()"<<endl);
	traceMap[index]=op;
	traceLength++;
	map<int, Thread> aState = map<int, Thread>(threadMap);
	saveState(index, aState);
	addInstruction(op->getThreadId(), index);
	if(isSomeLockActive()){
		for(map<long,int>::iterator it = locker.begin(); it != locker.end(); ++it){
			addLockState(make_pair(index,it->first), it->second);
		}
	}
}

Operation *getOperation(int index){
	if(traceMap.find(index) == traceMap.end())
		return NULL;
	Operation *op = traceMap.find(index)->second;
	return op;
}

void dumpTrace(){
	map<int, Operation*>::iterator it = traceMap.begin();
	for( ; it != traceMap.end(); ++it){
		it->second->dumpOpInfo();
		cerr<<endl<<endl<<endl;
	}
}


/******* Book-keeping for explicitDependenciesStateMap *******/

void addExplicitDependenciesState(int index, ExplicitDependencies obj){
	explicitDependenciesStateMap[index] = obj;
}

ExplicitDependencies getExplicitDependenciesState(int index){
	map<int, ExplicitDependencies> :: iterator it = explicitDependenciesStateMap.find(index);
	ExplicitDependencies emptyObj;
	if(it != explicitDependenciesStateMap.end())
		return it->second;
	return emptyObj;
}


/******* Book-Keeping for state *******/

void saveState(int index, map<int, Thread> state){
	stateMap[index]=state;
}

// a forward declaration
Thread getThreadState(int stateIndex, int threadId);

void dumpThreadState(int index, int threadId){
	Thread t = getThreadState(index,threadId);
	t.dumpThread();
}

map<int, Thread>  getState(int index){
	map<int, map<int, Thread> >::iterator it = stateMap.find(index);
	map<int, Thread> emptyState;
	if(it==stateMap.end()){
		return emptyState;
	}
	return it->second;
}

Thread getThreadState(int stateIndex, int threadId){
	//	Debug(dumpThreadState(stateIndex,threadId));
	Thread nullThread;
	map<int, Thread> threadStateMap = getState(stateIndex);
	if(threadStateMap.empty()){
		return nullThread;
	}
	map<int, Thread>::iterator it = threadStateMap.find(threadId);
	if(it==threadStateMap.end()){
		return nullThread;
	}
	return it->second;
}

//a forward declaration
bool isThreadEnabledForExecution(int stateIndex, int threadId, Thread threadState);


//checks if an operation is executable on a thread in given state(index).
bool isThreadEnabled(int stateIndex, int threadId, Thread threadState){
	DebugSelective(cerr<<"fn:isThreadEnabled :: beginning, state : "<<stateIndex<<", threadId : "<<threadId<<endl);
	if(threadState.isNullThread())
		return false;
	if(threadState.isLooping() && threadState.getExecutableTask() == -1)
		return false;

	int nextInstructionIndex = getNextInstruction(threadId, stateIndex);
	DebugSelective(cerr<<"fn:isThreadEnabled() :: nextInstructionIndex "<<nextInstructionIndex<<endl);


	if(nextInstructionIndex != -1){
		Operation *op = getOperation(nextInstructionIndex);
		if(op->getOpType() == LOCK){
			DebugSelective(cerr<<"fn:isThreadEnabled(): nextInst is lock"<<endl);
			LockOperation *lop = dynamic_cast<LockOperation *>(op);
			Debug(cerr<<"lock obj : "<<lop->getObjectId()<<endl);
			int lockHolderThreadId = getLockHolderThreadId(make_pair(stateIndex, lop->getObjectId()));
			DebugSelective(cerr<<"fn:isThreadEnabled() ::  lockHolderThreadId :  "<< lockHolderThreadId <<endl);
			if(lockHolderThreadId != -1 && lockHolderThreadId != threadId){
				return false;
			}
		}
		else if(op->getOpType() == WAIT){
			Thread thread = getThreadState(stateIndex, op->getThreadId());
			return (!thread.getNotifyCount() <= 0);
		}
		else if(op->getOpType() == POST || op->getOpType() == DELAY_POST || op->getOpType() == NATIVE_POST || op->getOpType() == UI_POST){
			PostOperation *pop = dynamic_cast<PostOperation *>(op);
			int targetThreadId = pop->getDestThreadId();
			Thread targetThread = getThreadState(stateIndex, targetThreadId);
			if(targetThread.isNullThread()){
				return false;
			}

			if(!targetThread.isQAttached()){
				return false;
			}
		}
		else if(op->getOpType() == NOP){
			int opIndex = mapContolLocationToProgramIndex(op->getOpIndex());
			if(getExplicitDependenciesState(stateIndex).isDependent(opIndex)){
				return false;
			}
		}
	}
	else{
		int prevInstIndexBeforeInputState = getPreviousInstruction(threadId, stateIndex, threadState);

		// below index is a program index, not a control location (because this operation is not yet executed in the current sequence)
		int nextProgInst = getNextInstructionForExecution(threadState, prevInstIndexBeforeInputState);

		if(nextProgInst == -1)
			return false;
		else{
			Operation *op = getOperationClone(nextProgInst);
			DebugSelective(cerr<<"dumping dependent operation info : "<<endl);
			DebugSelective(op->dumpOpInfo());
			if(op->getOpType() == LOCK){
				DebugSelective(cerr<<"fn:isThreadEnabled(): nextInst is lock"<<endl);
				LockOperation *lop = dynamic_cast<LockOperation *>(op);
				DebugSelective(cerr<<"lock obj : "<<lop->getObjectId()<<endl);
				int lockHolderThreadId = getLockHolderThreadId(make_pair(stateIndex, lop->getObjectId()));
				DebugSelective(cerr<<"fn:isThreadEnabled() ::  lockHolderThreadId :  "<< lockHolderThreadId <<endl);
				if(lockHolderThreadId != -1 && lockHolderThreadId != threadId){
					DebugSelective(cerr<<endl<<"-----Lock Blocked Thread-----"<<endl;
					cerr<<"Thread id : "<<op->getThreadId()<<" is blocked as lock is held by thread id : "<<lockHolderThreadId<<endl;
					cerr<<"Next Operation index : "<<op->getOpIndex()<<endl);
					delete op;
					return false;
				}
			}
			else if(op->getOpType() == WAIT){
				int waitingThreadId = op->getThreadId();
				delete op;
				return isWaitNotified(waitingThreadId);
			}
			else if(op->getOpType() == POST || op->getOpType() == DELAY_POST || op->getOpType() == NATIVE_POST || op->getOpType() == UI_POST){
				PostOperation *pop = dynamic_cast<PostOperation *>(op);
				int targetThreadId = pop->getDestThreadId();
				Thread targetThread = getThreadState(stateIndex, targetThreadId);
				if(targetThread.isNullThread()){
					delete op;
					return false;
				}
				if(!targetThread.isQAttached()){
					delete op;
					return false;
				}
			}
			else if(op->getOpType() == NOP){
				if(getExplicitDependenciesState(stateIndex).isDependent(op->getOpIndex())){
					delete op;
					return false;
				}
			}
			delete op;
		}
	}
	return true;
}

vector<int> getEnabledThreadsInBacktrackCall(int stateIndex){
	map<int, Thread> state = getState(stateIndex);
	vector<int> resultVector;
	if(state.empty())
		return resultVector;
	for(map<int, Thread>::iterator it = state.begin(); it != state.end(); ++it){
		Debug(cerr<<"fn:getEnabledThreadsInBacktrackCall() :: cheking if threadId "<<it->first<<" is enabled ?? "<<endl);
		if(isThreadEnabled(stateIndex, it->first, it->second)){
			Debug(cerr<<"fn:getEnabledThreadsInBacktrackCall() :: threadId "<<it->first<<" is enabled !!! "<<endl);
			resultVector.push_back(it->first);
		}
	}
	return resultVector;
}


// to be used in fn:Execute()
bool isThreadEnabledForExecution(int stateIndex, int threadId, Thread threadState){
	if(threadState.isNullThread())
		return false;
	int nextInstructionIndex=-1;
	map<int, vector<int> >::iterator it = threadIndicesMap.find(threadId);
	if(it != threadIndicesMap.end()){
		vector<int> threadIndices = it->second;
		if(!threadIndices.empty()){
			vector<int>::reverse_iterator rit = threadIndices.rbegin();
			int afterIndex = *rit;
			nextInstructionIndex = getNextInstructionForExecution(threadState, afterIndex);
		}
		else{
			cerr<<"ERROR :: in fn:isThreadEnabledForExecution(), threadIndices shouldn't have been empty!"<<endl;
			throw error;
		}
	}
	else{

		if(getParentForkIndex(threadId) != -1){				//Thread has been forked but not yet initialized!
			nextInstructionIndex = getNextInstructionForExecution(threadState, -1);		//Need to return 1st op of this thread
		}
		else{
			nextInstructionIndex = getNextInstructionForExecution(threadState, stateIndex);	
		}
	}
	DebugSelective(cerr<<"fn:isThreadEnabledForExecution() :: nextInstructionIndex "<<nextInstructionIndex<<endl);
	if(nextInstructionIndex == -1)
		return false;
	else{
		Operation *op = getOperationClone(nextInstructionIndex);
		DebugSelective(op->dumpOpInfo());
		if(op->getOpType() == LOCK){
			DebugSelective(cerr<<"fn:isThreadEnabled(): nextInst is lock"<<endl);
			LockOperation *lop = dynamic_cast<LockOperation *>(op);
			DebugSelective(cerr<<"lock obj : "<<lop->getObjectId()<<endl);
			int lockHolderThreadId = getLockHolderThreadId(make_pair(stateIndex, lop->getObjectId()));
			DebugSelective(cerr<<"fn:isThreadEnabled() ::  lockHolderThreadId :  "<< lockHolderThreadId <<endl);
			if(lockHolderThreadId != -1 && lockHolderThreadId != threadId){
				DebugSelective(cerr<<endl<<"-----Lock Blocked Thread-----"<<endl;
				cerr<<"Thread id : "<<op->getThreadId()<<" is blocked as lock is held by thread id : "<<lockHolderThreadId<<endl;
				cerr<<"Next Operation index : "<<op->getOpIndex()<<endl);
				delete op;
				return false;
			}
		}
		else if(op->getOpType() == WAIT){
			int waitingThreadId = op->getThreadId();
			delete op;
			return isWaitNotified(waitingThreadId);
		}
		else if(op->getOpType() == POST || op->getOpType() == DELAY_POST || op->getOpType() == NATIVE_POST || op->getOpType() == UI_POST){
			PostOperation *pop = dynamic_cast<PostOperation *>(op);
			int targetThreadId = pop->getDestThreadId();
			Thread targetThread = getThreadState(stateIndex, targetThreadId);
			if(targetThread.isNullThread()){
				delete op;
				return false;
			}
			if(!targetThread.isQAttached()){
				delete op;
				return false;
			}
		}
		else if(op->getOpType() == NOP){
			if(getExplicitDependenciesState(stateIndex).isDependent(op->getOpIndex())){
				delete op;
				return false;
			}
		}

		delete op;
	}

	return true;
}

vector<int> getEnabledThreads(int stateIndex){
	map<int, Thread> state = getState(stateIndex);
	vector<int> resultVector;
	if(state.empty())
		return resultVector;
	for(map<int, Thread>::iterator it = state.begin(); it != state.end(); ++it){
		DebugSelective(cerr<<"fn:getEnabledThreads() :: cheking if threadId "<<it->first<<" is enabled ?? "<<endl);
		if(isThreadEnabledForExecution(stateIndex, it->first, it->second)){
			DebugSelective(cerr<<"fn:getEnabledThreads() :: threadId "<<it->first<<" is enabled !!! "<<endl);
			resultVector.push_back(it->first);
		}
	}
	return resultVector;
}

vector<int> getAllThreads(int state){
	map<int, Thread> threadState = getState(state);
	vector<int> resultVector;
	for(map<int, Thread>::iterator it = threadState.begin(); it != threadState.end(); ++it)
		resultVector.push_back(it->first);
	return resultVector;
}

bool isThreadLooping(Thread threadState){
	if(threadState.isNullThread())
		return false;
	return threadState.isLooping();
}

bool isTaskExecutable(int taskId, Thread threadState){
	//	threadState.dumpThread();
	if(taskId == -1 && !threadState.isLooping())
		return true;
	if(taskId == threadState.getTaskId())
		return true;
	if(threadState.getTaskId() == -1 && threadState.isLooping() && taskId == threadState.getFrontOfQueue())
		return true;

	return false;
}


void updateThreadQ(int index, int threadId, int taskId){
	Thread t = getThreadState(index, threadId);
	t.dumpThread();
	if(t.isNullThread()){
		return;
	}
	t.insertIntoQ(taskId);

	return;
}



/******* Book-Keeping for Fork Operations *******/

//arguments: 1-thread id of the new thread, 2-index of fork operation
void addFork(int forkedThreadId, int forkOpIndex){
	forkMap[forkedThreadId]=forkOpIndex;
}

int getParentForkIndex(int threadId){
	map<int,int>::iterator it= forkMap.find(threadId);
	if(it == forkMap.end())
		return -1;									//Not found
	return it->second;
}

void removeFork(int threadId){
	forkMap.erase(threadId);
}



/******* Book-Keeping for Post Operations *******/


void addPost(int postedMessageId, int postOpIndex){
	//	Debug(cerr<<"fn:getParentPostIndex(), Input recieved:: msgId : "<<postedMessageId<<" index : "<<postOpIndex<<endl);
	postMap[postedMessageId]=postOpIndex;
}

int getParentPostIndex(int msgId){
	//	Debug(cerr<<"fn:getParentPostIndex(), Input recieved : "<<msgId<<endl);
	map<int,int>::iterator it = postMap.find(msgId);
	if(it==postMap.end()){
		return -1;
	}
	return it->second;
}


/******* Post-ops & their target Threads *******/

vector < pair<int, int> > getPostsToThread(int targetThreadId){
	map < int, vector < pair<int, int> > > :: iterator it = threadPostMap.find(targetThreadId);
	vector < pair<int,int> > postVector;
	if(it!=threadPostMap.end())
		postVector = it->second;
	return postVector;
}

// key - TargetThreadId, value - pair ( postIndex, threadFromWherePost was made)
void addToThreadPostMap(int targetThreadId, int postOpIndex, int postingThreadId){
	vector < pair<int,int> > postVector = getPostsToThread(targetThreadId);
	postVector.push_back(make_pair(postOpIndex, postingThreadId));
	threadPostMap[targetThreadId] = postVector;
}


/******* Book-Keeping for Post-Hook Operations - Hooks are removed in END operation handler. *******/

void addPostHook(int postedMessageId, int postOpIndex, int destThreadId, int postingThreadId){
	postHookMap[postedMessageId]=postOpIndex;
	addPost(postedMessageId, postOpIndex);
	addToThreadPostMap(destThreadId, postOpIndex, postingThreadId);
}

int getParentPostHookIndex(int msgId){
	map<int,int>::iterator it = postHookMap.find(msgId);
	if(it==postHookMap.end()){
		return -1;
	}
	return it->second;
}

void removePostHook(int msgId){
	postHookMap.erase(msgId);
}


/******* Book-Keeping for Enable Operations *******/

void addEnable(const pair<long, string> key, int enableOpIndex){
	enableMap.insert(pair<pair<long, string>, int> (key, enableOpIndex));
}

int getParentEnableIndex(pair<long, string> key){
	map< pair<long, string>, int, compareEnableKey >::iterator it = enableMap.find(key);
	if(it==enableMap.end()){
		return -1;
	}
	return it->second;
}

void removeEnable(pair<long, string> key){
	enableMap.erase(key);
}


/******* Book-Keeping for READ/WRITE Operations *******/

void addHeapRead(pair<long,int> key, int memAccessIndex){
	map< pair<long,int>, vector<int>, compareReadWrtieKey>::iterator it = heapReadMap.find(key);
	vector<int> accessVector;
	if(it!=heapReadMap.end())
		accessVector = it->second;
	accessVector.push_back(memAccessIndex);
	heapReadMap[key]=accessVector;
}

vector<int> getHeapReadIndices(pair<long,int> key){			//if key not present, returns empty vector!
	map< pair<long,int>, vector<int>, compareReadWrtieKey >::iterator it = heapReadMap.find(key);
	if(it==heapReadMap.end()){
		vector<int> v;
		return v;
	}
	return it->second;
}

void addHeapWrite(pair<long,int> key, int memAccessIndex){
	map< pair<long,int>, vector<int>, compareReadWrtieKey>::iterator it = heapWriteMap.find(key);
	vector<int> accessVector;
	if(it!=heapWriteMap.end()){
		accessVector = it->second;
	}
	accessVector.push_back(memAccessIndex);
	heapWriteMap[key]=accessVector;
}

void dumpheapWriteMap(){
	map< pair<long,int>, vector<int>, compareReadWrtieKey>::iterator it = heapWriteMap.begin();
	for( ; it!=heapWriteMap.end(); ++it){
		Debug(cerr<<"obj : "<<it->first.first<<" field : "<<it->first.second<<" index : ");
		int i=0;
		for(vector<int>::iterator vit = it->second.begin(); vit!=it->second.end() && i<5; ++vit){
			Debug(cerr<<" "<<*vit);
			i++;
		}
		Debug(cerr<<endl);
	}
}

vector<int> getHeapWriteIndices(pair<long,int> key){			//if key not present, returns empty vector!
	map< pair<long,int>, vector<int>, compareReadWrtieKey >::iterator it = heapWriteMap.find(key);
	if(it==heapWriteMap.end()){
		vector<int> v;
		return v;
	}
	return it->second;
}

/******* Book-Keeping for READ-STATIC/WRITE-STATIC Operations *******/

void addStackRead(pair<string,int> key, int memAccessIndex){
	map< pair<string,int>, vector<int>, compareStaticAccessKey>::iterator it = stackReadMap.find(key);
	vector<int> accessVector;
	if(it!=stackReadMap.end())
		accessVector = it->second;
	accessVector.push_back(memAccessIndex);
	stackReadMap[key]=accessVector;
}

vector<int> getStackReadIndices(pair<string,int> key){			//if key not present, returns empty vector!
	map< pair<string,int>, vector<int>, compareStaticAccessKey >::iterator it = stackReadMap.find(key);
	if(it==stackReadMap.end()){
		vector<int> v;
		return v;
	}
	return it->second;
}

void dumpStackWriteMap(){
	map< pair<string,int>, vector<int>, compareStaticAccessKey >::iterator it = stackWriteMap.begin();
	for( ; it!=stackWriteMap.end(); ++it){
		Debug(cerr<<"class : "<<it->first.first<<" field : "<<it->first.second<<" index : ");
		int i=0;
		for(vector<int>::iterator vit = it->second.begin(); vit!=it->second.end() && i<5; ++vit){
			Debug(cerr<<" "<<*vit);
			i++;
		}
		Debug(cerr<<endl);
	}
}

void addStackWrite(pair<string,int> key, int memAccessIndex){
	map< pair<string,int>, vector<int>, compareStaticAccessKey>::iterator it = stackWriteMap.find(key);
	vector<int> accessVector;
	if(it!=stackWriteMap.end())
		accessVector = it->second;
	accessVector.push_back(memAccessIndex);
	stackWriteMap[key]=accessVector;
}

vector<int> getStackWriteIndices(pair<string,int> key){			//if key not present, returns empty vector!
	map< pair<string,int>, vector<int>, compareStaticAccessKey >::iterator it = stackWriteMap.find(key);
	if(it==stackWriteMap.end()){
		vector<int> v;
		return v;
	}
	return it->second;
}

/******* Book-Keeping last operation index of a task for same task HB edges. *******/

void addLastOpIndex(int taskId, int lastOpIndex){			//also used for updating!
	taskLastOpMap[taskId]=lastOpIndex;
}

int getLastOpIndex(int taskId){
	map<int,int>::iterator it = taskLastOpMap.find(taskId);
	if(it==taskLastOpMap.end())
		return -1;
	return it->second;
}

void removeLastOpIndex(int taskId){
	taskLastOpMap.erase(taskId);
}

/******* Book-Keeping last operation index of a thread for same thread HB edges. *******/

void addThreadLastOpIndex(int threadId, int lastOpIndex){			//also used for updating!
	threadLastOpMap[threadId]=lastOpIndex;
}

int getThreadLastOpIndex(int threadId){
	map<int,int>::iterator it = threadLastOpMap.find(threadId);
	if(it==threadLastOpMap.end()){
		Debug(if(!threadLastOpMap.empty()) cerr<<"Map is not empty"<<endl);
		return -1;
	}
	return it->second;
}

void removeThreadLastOpIndex(int threadId){
	threadLastOpMap.erase(threadId);
}


/******* Book-Keeping attachQ operation index of a thread for attachQ to Post HB edges. *******/

void addAttachQIndex(int threadId, int index){
	attachQIndexMap[threadId]=index;
}

int getAttachQIndex(int threadId){
	map<int,int>::iterator it = attachQIndexMap.find(threadId);
	if(it==attachQIndexMap.end())
		return -1;
	else
		return it->second;
}

void removeAttachQIndex(int threadId){
	attachQIndexMap.erase(threadId);
}


/******* Book-Keeping attachQ operation index of a thread for LOOP to BEGIN HB edges. *******/

void addLoopIndex(int threadId, int index){
	loopIndexMap[threadId]=index;
}

int getLoopIndex(int threadId){
	map<int,int>::iterator it = loopIndexMap.find(threadId);
	if(it==loopIndexMap.end())
		return -1;
	else
		return it->second;
}

void removeLoopIndex(int threadId){
	loopIndexMap.erase(threadId);
}


/******* Book-Keeping for LOCK/UNLOCK Operations *******/

void addLockHook(long objectId, int index){
	map< long, int>::iterator it = lockUnlockMap.find(objectId);
	if(it==lockUnlockMap.end())
		lockUnlockMap[objectId] = index;
	else{
		cerr<<"ERROR: trying to take a lock when it is already locked."<<endl;
		throw error;
	}
}

int getLockForUnlock(long objectId){
	map< long, int>::iterator it = lockUnlockMap.find(objectId);
	int lockIndex = -1;
	if(it==lockUnlockMap.end()){
		cerr<<"ERROR: trying to release a lock when it is not locked."<<endl;
		throw error;
	}
	else{
		lockIndex = it->second;
		lockUnlockMap.erase(objectId);
	}
	return lockIndex;
}


void addUnlockHook(long objectId, int index){
	map<long, vector<int> >::iterator it = unlockMap.find(objectId);
	vector<int> unlockVector;
	if(it!=unlockMap.end())
		unlockVector = it->second;
	unlockVector.push_back(index);
	unlockMap[objectId] = unlockVector;
}

}
#endif /* BOOKKEEPING_H_ */
