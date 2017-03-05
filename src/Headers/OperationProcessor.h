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


#ifndef OPERATIONPROCESSOR_H_
#define OPERATIONPROCESSOR_H_

#include <stdlib.h>
#include <iostream>
#include <string>


#include <ComputeBacktrack.h>
#include <ComputeBacktrackDPOR.h>

using namespace std;
using namespace emdpor_Entities;
using namespace emdpor_bookKeeper;
using namespace emdpor_vc;

namespace emdpor_OperationProcessor{

// pass true if call originated from replay()
void addSameThreadOrTaskHBEdge(Operation *op, bool isReplay, bool isDPOR){
	Thread t=getThread(op->getThreadId());
	if(!t.isNullThread()){
		if(t.isLooping()){
			op->setTaskId(t.getTaskId());
			if(!isReplay)
				addHBEdge(getOperation(getLastOpIndex(t.getTaskId())),op,isDPOR);
			addLastOpIndex(t.getTaskId(),op->getOpIndex());
		}
		else{
			if(!isReplay){
				addHBEdge(getOperation(getThreadLastOpIndex(t.getThreadId())),op,isDPOR);
			}
			addThreadLastOpIndex(t.getThreadId(), op->getOpIndex());
		}
	}
}

//void addSameThreadOrTaskHBEdgeForPartialFn(Operation *op){
//	Debug(cerr<<"addSameThreadOrTaskHBEdgeForPartialFn() :: begin, thread id = "<<op->getThreadId()<<endl);
//	Thread t=getThread(op->getThreadId());
//	if(!t.isNullThread()){
//		if(t.isLooping()){
//			Debug(cerr<<"thread is looping "<<endl);
//			op->setTaskId(t.getTaskId());
//			//			addHBEdge(getLastOpIndex(t.getTaskId()),op->getOpIndex());
//			addTempHBEdges(getLastOpIndex(t.getTaskId()),op->getOpIndex());
//		}
//		else{
//			Debug(cerr<<"thread is not looping"<<endl);
//			//			Debug(cerr<<"HB Edge source thread id = "<<getThreadLastOpIndex(t.getThreadId())<<endl);
//			//			addHBEdge(getThreadLastOpIndex(t.getThreadId()),op->getOpIndex());
//			addTempHBEdges(getThreadLastOpIndex(t.getThreadId()),op->getOpIndex());
//		}
//	}
//	Debug(cerr<<"addSameThreadOrTaskHBEdgeForPartialFn() :: end"<<endl);
//}

int replayCounter=0;
char * logFilePath;
bool singleFile = false;
void writeLog(Operation *op){
	ofstream myfile;
	myfile.open (exploredTraceFile.c_str(), ios::app);
	if(op->getOpIndex() == 0){
		replayCounter++;
		if(replayCounter == 1){
			myfile << "\t\t\t\t\t\t\t\t\t ---| TRACE - "<<replayCounter <<" |---- "<<endl;
			myfile << endl;
		}
		else{
			myfile << endl;
			myfile << endl;
			myfile << "\t\t\t\t\t\t\t\t\t ---| TRACE - "<<replayCounter <<" |---- "<<endl;
			myfile << endl;
		}

	}
	if(op->getOpType() == BEGIN)
		myfile << endl;
	// This one formats output to be readable but not useful for trace having more than 6 threads.
	myfile << op->writeToLogWithFormatting() <<endl;

	// without formatting, for trace with too many threads
	//myfile << op->writeToLog() <<endl;
	myfile.close();
}

void setExploredTraceFilePathForSingleFile(char* filePath){
	exploredTraceFile = filePath;
	ofstream myfile;
	myfile.open (filePath);
	myfile.close();
	fileOrFolderOption=true;
}

void writeLogToMultipleFiles(Operation *op){
	ofstream myfile;
	if(op->getOpIndex() == 0){
		replayCounter++;
	}
	std::stringstream sstm;
	sstm <<exploredTraceFolder << "/trace" << replayCounter;

	/*
	 * ********************************  IMPORTANT ******************************** *
	 *
	 * const char * filname = sstm.str().c_str();
	 *
	 * The above line can cause weird behavior as sstm.str() returns a temporary string
	 * The memory for such a string is freed just after this instrcution in which case
	 * .c_str() returned pointer would be pointing to garbage. So, rather assign the string
	 * returned by sstm.str() to some temporary string variable first & then get its char pointer.
	 *
	 */

	string temp = sstm.str();
	const char * filname = temp.c_str();
	myfile.open (filname, ios::app);

	if(op->getOpType() == BEGIN)
		myfile << endl;
	// This one formats output to be readable but not useful for trace having more than 6 threads.
	myfile << op->writeToLogWithFormatting() <<endl;

	// without formatting, for trace with too many threads
//	myfile << op->writeToLog() <<endl;
	myfile.close();
}

void setExploredTraceFolderPathForMultipleFiles(char* path){
	exploredTraceFolder = path;
	fileOrFolderOption=false;
}

void processOperation(Operation *op, bool isDPOR, bool isCalledFromReplay){

	/*
	 * if switching off HB optimization -
	 * 1. in case POST: comment the !isCalledFromReplay part of the condition when switching off the HB-optimization.
	 *
	 */

	//switching off HB-optimization - earlier we had an optimization which would not recompute HB for replay.  we dont do that anymore
	//and recompute HB even for replay.
	isCalledFromReplay=false;

	Debug(cerr<<"index : "<<op->getOpIndex()<<" \t Processing Operation : "<<enumOpName[op->getOpType()] << endl);
	//dumpHBGraph();
	if(op==NULL) { cerr<<"NULL passed as operation in fn:processOperation() !!"<<endl; throw error;}

	Write(
	if(fileOrFolderOption)
		writeLog(op);
	else
		writeLogToMultipleFiles(op));

	int opId = op->getOpType();
	int opIndex = op->getOpIndex();
	LifecycleOperation *lOp;
	Attach_QOperation* aOp;
	PostOperation* pOp;
	ForkOperation *fOp;
	MemoryAccessOperation * mOp;
	LockOperation *lockOp;
	NotifyOperation *nOp;
	int index;
	int dequedTask;
	Thread & t = getThread(op->getThreadId());
	//	if(t.isNullThread()){
	//		Debug(cerr<<"ERROR :: getThread returned a NULL Thread....!!  Returning empty");
	//		return;
	//	}
	switch(opId){
	case START:
		// Do Nothing
		break;
	case THREAD_INIT:
		addThread(op->getThreadId());
		addThreadLastOpIndex(op->getThreadId(), opIndex);
		index=getParentForkIndex(op->getThreadId());
		if(index!=-1){
			if(!isCalledFromReplay){
				addHBEdge(getOperation(index), op, isDPOR);
			}
			removeFork(op->getThreadId());				//remove fork hook
		}
		addOperation(opIndex, op);
		break;
	case THREAD_EXIT:									//TO DO : Re-Think about it!
		if(!t.isNullThread()){
			if(!t.isLooping()){
				Debug(cerr<<getThreadLastOpIndex(t.getThreadId())<<endl);
				if(!isCalledFromReplay){
					addHBEdge(getOperation(getThreadLastOpIndex(t.getThreadId())), op, isDPOR);
				}
				addThreadLastOpIndex(t.getThreadId(), op->getOpIndex());
			}
		}
		removeAttachQIndex(op->getThreadId());
		removeLoopIndex(op->getThreadId());
		removeThreadLastOpIndex(t.getThreadId());
		removeThread(op->getThreadId());
		addOperation(opIndex, op);
		break;
	case FORK:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay, isDPOR);
		fOp = dynamic_cast<ForkOperation *>(op);
		addFork(fOp->getChildThreadId(), opIndex);
		addThread(fOp->getChildThreadId(), fOp->getThreadId());
		//		if(!t.isLooping())
		//			addThreadLastOpIndex(t.getThreadId(), opIndex);
		addOperation(opIndex, fOp);
		break;
	case ATTACH_Q:
		addAttachQIndex(t.getThreadId(),opIndex);
		if(!isCalledFromReplay){
			addHBEdge(getOperation(getThreadLastOpIndex(t.getThreadId())), op, isDPOR);
		}
		addThreadLastOpIndex(t.getThreadId(), opIndex);
		aOp = dynamic_cast<Attach_QOperation *>(op);
		t.attachQ(aOp->getQueueId());
		addOperation(opIndex, aOp);
		break;
	case LOOP:
		t.setLooper(true);
		addLoopIndex(t.getThreadId(),opIndex);
		if(!isCalledFromReplay){
			addHBEdge(getOperation(getThreadLastOpIndex(t.getThreadId())), op, isDPOR);
		}
		removeThreadLastOpIndex(t.getThreadId());
		aOp = dynamic_cast<Attach_QOperation *>(op);
		addOperation(opIndex, aOp);
		break;
	case POST:
	case NATIVE_POST:
	case UI_POST:
	case DELAY_POST:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay, isDPOR);
		pOp = dynamic_cast<PostOperation *>(op);

		//comment the !isCalledFromReplay part of the condition when switching off the HB-optimization.
		if(isDPOR && !isCalledFromReplay){
			addPostHBEdgesInDPOR(pOp, isDPOR);
		}
		if(!isCalledFromReplay){
			addAttachqPostHBEdge(getOperation(getAttachQIndex(pOp->getDestThreadId())), pOp, isDPOR);		// AttachQ ---HB---> Post
		}
		addPostHook(pOp->getPostedTaskId(), opIndex, pOp->getDestThreadId(), pOp->getThreadId());
		updateQueue(pOp->getDestThreadId(), pOp->getPostedTaskId());
		addOperation(opIndex, pOp);
		break;
	case BEGIN:
		t.setTaskId(op->getTaskId());
		Debug(t.dumpQueue());
		dequedTask = t.deque();
		if(dequedTask!=op->getTaskId()){
			cerr<<" ERROR :: Begin dequeued a task not at the front of queue! dequed task ID :: "<< dequedTask <<endl;
			t.dumpQueue();
			throw error;
		}
		addLastOpIndex(op->getTaskId(),opIndex);
		index=getParentPostHookIndex(op->getTaskId());
		if(index!=-1){
			pOp = dynamic_cast<PostOperation *>(getOperation(index));
			pOp->setBeginOpIndex(opIndex);
			if(!isCalledFromReplay){
				addHBEdge(getOperation(index), op, isDPOR);										// Post ---HB---> Begin
				addFifoHBEdges(op, isDPOR);
				addHBEdge(getOperation(getLoopIndex(op->getThreadId())), op, isDPOR);			// Loop ---HB---> Begin
			}
		}
		addOperation(opIndex, op);
		break;
	case END:
		if(!isCalledFromReplay){
			addHBEdge(getOperation(getLastOpIndex(t.getTaskId())), op, isDPOR);
		}

		index=getParentPostIndex(op->getTaskId());
		if(index!=-1){
			pOp = dynamic_cast<PostOperation *>(getOperation(index));
			pOp->setEndOpIndex(opIndex);
			removePostHook(op->getTaskId());
		}
		removeLastOpIndex(t.getTaskId());
		t.setTaskId(-1);			//Marking that thread is not running any task now.
		addOperation(opIndex, op);
		break;
	case ENABLE_LIFECYCLE:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay, isDPOR);
		lOp = dynamic_cast<LifecycleOperation *>(op);
		addEnable(make_pair(lOp->getId(),lOp->getState()), opIndex);
		addOperation(opIndex, lOp);
		break;
	case TRIGGER_LIFECYCLE:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay, isDPOR);
		lOp = dynamic_cast<LifecycleOperation *>(op);
		//		Debug(cerr<<"TRIGGER_LIFECYCLE: looking for key:: Id : "<<lOp->getId()<<" State: "<<lOp->getState()<<endl);
		index=getParentEnableIndex(make_pair(lOp->getId(), lOp->getState()));
		//		Debug(cerr<<"index returned is : "<<index<<endl);
		if(index!=-1){
			if(!isCalledFromReplay){
				addHBEdge(getOperation(index), op, isDPOR);
			}
			removeEnable(make_pair(lOp->getId(), lOp->getState()));		//remove enable hook
		}
		addOperation(opIndex, op);
		break;
	case ENABLE_EVENT:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay, isDPOR);
		addOperation(opIndex, op);
		break;
	case TRIGGER_EVENT:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay, isDPOR);
		addOperation(opIndex, op);
		break;
	case ENABLE_WINDOW_FOCUS:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay, isDPOR);
		addOperation(opIndex, op);
		break;
	case TRIGGER_WINDOW_FOCUS:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay, isDPOR);
		addOperation(opIndex, op);
		break;
	case TRIGGER_BROADCAST:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay, isDPOR);
		addOperation(opIndex, op);
		break;
	case TRIGGER_SERVICE:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay, isDPOR);
		addOperation(opIndex, op);
		break;
	case NOP:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay, isDPOR);
		addOperation(opIndex, op);
		break;
	case READ:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay, isDPOR);
		mOp = dynamic_cast<MemoryAccessOperation *>(op);
		if(!isCalledFromReplay){
			addHeapWriteHBEdges(make_pair(mOp->getObjectId(), mOp->getField()), op, isDPOR);
		}
		addHeapRead(make_pair(mOp->getObjectId(), mOp->getField()), opIndex);
		addOperation(opIndex, op);
		break;
	case WRITE:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay, isDPOR);
		mOp = dynamic_cast<MemoryAccessOperation *>(op);
		if(!isCalledFromReplay){
			addHeapWriteHBEdges(make_pair(mOp->getObjectId(), mOp->getField()), op, isDPOR);
			addHeapReadHBEdges(make_pair(mOp->getObjectId(), mOp->getField()), op, isDPOR);
		}
		addHeapWrite(make_pair(mOp->getObjectId(), mOp->getField()), opIndex);
		addOperation(opIndex, op);
		break;
	case READ_STATIC:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay, isDPOR);
		mOp = dynamic_cast<MemoryAccessOperation *>(op);
		if(!isCalledFromReplay){
			addStackWriteHBEdges(make_pair(mOp->getClassName(), mOp->getField()), op, isDPOR);
		}
		addStackRead(make_pair(mOp->getClassName(), mOp->getField()), opIndex);
		addOperation(opIndex, op);
		break;
	case WRITE_STATIC:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay, isDPOR);
		mOp = dynamic_cast<MemoryAccessOperation *>(op);
		if(!isCalledFromReplay){
			addStackWriteHBEdges(make_pair(mOp->getClassName(), mOp->getField()), op, isDPOR);
			addStackReadHBEdges(make_pair(mOp->getClassName(), mOp->getField()), op, isDPOR);
		}
		addStackWrite(make_pair(mOp->getClassName(), mOp->getField()), opIndex);
		addOperation(opIndex, op);
		break;
	case LOCK:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay, isDPOR);
		lockOp = dynamic_cast<LockOperation *>(op);
		addLockHook(lockOp->getObjectId(), lockOp->getOpIndex());
		if(!isCalledFromReplay){
			addUnlockToLockHBEdges(lockOp->getObjectId(), lockOp->getThreadId(), op, isDPOR);
		}
		addLockOperation(opIndex, lockOp, lockOp->getObjectId(), lockOp->getThreadId());
		addLock(lockOp->getObjectId(), opIndex);

		break;
	case UNLOCK:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay, isDPOR);
		lockOp = dynamic_cast<LockOperation *>(op);
		index = getLockForUnlock(lockOp->getObjectId());
		if(index != -1 && !isCalledFromReplay){
			addHBEdge(getOperation(index), op, isDPOR);
		}
		else if(index == -1){
			cerr<<"ERROR: No lock for unlock operation, unlock index = "<<lockOp->getOpIndex();
			throw error;
		}
		addUnlockOperation(opIndex, lockOp, lockOp->getObjectId());
		addUnlockHook(lockOp->getObjectId(),opIndex);
		break;
	case WAIT:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay, isDPOR);
		index = popNotifyOperation(op->getThreadId());
		if(index!=-1 && !isCalledFromReplay){
			addHBEdge(getOperation(index), op, isDPOR);
		}
		else if(index==-1){
			cerr<<"ERROR :: Couldn't find a Notify Operation to complete Wait operation at index : "<<opIndex<<endl;
			throw error;
		}
		decrementNotifyCount(op->getThreadId());
		addOperation(opIndex, op);
		break;
	case NOTIFY:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay, isDPOR);
		nOp = dynamic_cast<NotifyOperation *>(op);
		addNotifyOperation(nOp->getNotifyTagetThreadId(), opIndex);
		incrementNotifyCount(nOp->getNotifyTagetThreadId());
		addOperation(opIndex, op);
		break;
	}
}


//for DPOR do not consider 2 operations on the same thread "may be co-enabled"
int partialProcessOperation(Operation *op, bool isDPOR, int opProgramIndex){		// returns control location of last dependent operation
	Debug(if(op==NULL) cerr<<"NULL passed as operation in fn:processOperation() !!"<<endl);
	int opId = op->getOpType();
	int opIndex = op->getOpIndex();
	int threadId = op->getThreadId();
	Thread t=getThread(op->getThreadId());
	if(!t.isNullThread() && t.isLooping()){
		op->setTaskId(t.getTaskId());
	}
	MemoryAccessOperation * mOp;
	LockOperation *lockOp;
	PostOperation *postOp;
	int index;
	Debug(dumpClock());
	switch(opId){

	case READ:
		mOp = dynamic_cast<MemoryAccessOperation *>(op);
		index = getLastDependentHeapAccessInstIndexWithRead(make_pair(mOp->getObjectId(), mOp->getField()), opIndex, isDPOR, threadId, opProgramIndex, op);
		return index;
	case WRITE:
		mOp = dynamic_cast<MemoryAccessOperation *>(op);
		index = getLastDependentHeapAccessInstIndexWithWrite(make_pair(mOp->getObjectId(), mOp->getField()), opIndex, isDPOR, threadId, opProgramIndex, op);
		return index;
	case READ_STATIC:
		mOp = dynamic_cast<MemoryAccessOperation *>(op);
		index = getLastDependentStackAccessInstIndexWithRead(make_pair(mOp->getClassName(), mOp->getField()), opIndex, isDPOR, threadId, opProgramIndex, op);
		return index;
	case WRITE_STATIC:
		mOp = dynamic_cast<MemoryAccessOperation *>(op);
		index = getLastDependentStackAccessInstIndexWithWrite(make_pair(mOp->getClassName(), mOp->getField()), opIndex, isDPOR, threadId, opProgramIndex, op);
		return index;
	case LOCK:
		lockOp = dynamic_cast<LockOperation *>(op);
		index = getLastDependentLockInstIndex(lockOp->getObjectId(), threadId, isDPOR, op);
		return index;
	case POST:
	case UI_POST:
	case DELAY_POST:
	case NATIVE_POST:
		if(isDPOR){
			postOp = dynamic_cast<PostOperation *>(op);
			index = getLastPostToSameThreadInstIndex(postOp->getOpIndex(), postOp->getThreadId(), postOp->getDestThreadId(), opProgramIndex, isDPOR, op);
			return index;
		}
		else return -1;
	default:
		return -1;
	}
}

set<int> partialProcessWriteAndLockOperations(Operation *op, bool isDPOR, int opProgramIndex){
	Debug(if(op==NULL) cerr<<"NULL passed as operation in fn:partialProcessWriteOperation() !!"<<endl);
	int opIndex = op->getOpIndex(); //this is different from opProgramIndex passed as opIndex got updated before this function call was made
	int threadId = op->getThreadId();
	Thread t=getThread(op->getThreadId());
	if(!t.isNullThread() && t.isLooping()){
		op->setTaskId(t.getTaskId());
	}
	set<int> resultSet;
	MemoryAccessOperation * mOp;
	LockOperation *lockOp;

	switch(op->getOpType()){

	case WRITE:
		Debug(dumpClock());
		mOp = dynamic_cast<MemoryAccessOperation *>(op);
		resultSet = getAllDependentHeapAccessInstIndexWithWrite(make_pair(mOp->getObjectId(), mOp->getField()), opIndex, isDPOR, threadId, opProgramIndex, op);
		break;

	case WRITE_STATIC:
		Debug(dumpClock());
		mOp = dynamic_cast<MemoryAccessOperation *>(op);
		resultSet = getAllDependentStackAccessInstIndexWithWrite(make_pair(mOp->getClassName(), mOp->getField()), opIndex, isDPOR, threadId, opProgramIndex, op);
		break;

	case LOCK:
		lockOp = dynamic_cast<LockOperation *>(op);
		resultSet = getAllDependentLockInstOnAThreadUntilDiffThreadLockInterference(lockOp->getObjectId(), threadId, isDPOR, op);
		break;

	}

	return resultSet;
}


}

#endif /* OPERATIONPROCESSOR_H_ */
