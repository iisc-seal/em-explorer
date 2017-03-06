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
using namespace dpor_vc;

namespace emdpor_OperationProcessor{

// pass true if call originated from replay()
void addSameThreadOrTaskHBEdge(Operation *op, bool isReplay){
	Thread t=getThread(op->getThreadId());
	if(!t.isNullThread()){
		op->setTaskId(t.getTaskId());
		if(!isReplay){
			addHBEdge(getThreadLastOpIndex(t.getThreadId()),op->getOpIndex(),op->getThreadId());
		}
		addThreadLastOpIndex(t.getThreadId(), op->getOpIndex());
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
	if(op->getOpId() == BEGIN)
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

	if(op->getOpId() == BEGIN)
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

	//switching off HB-optimization
	isCalledFromReplay=false;

	Write(cerr<<"index : "<<op->getOpIndex()<<" \t Processing Operation : "<<enumOpName[op->getOpId()] << endl);
	//dumpHBGraph();
	if(op==NULL) { cerr<<"NULL passed as operation in fn:processOperation() !!"<<endl; throw error;}

	Write(
	if(fileOrFolderOption)
		writeLog(op);
	else
		writeLogToMultipleFiles(op));

	int opId = op->getOpId();
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
					addHBEdge(index, opIndex, op->getThreadId());
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
					addHBEdge(getThreadLastOpIndex(t.getThreadId()),op->getOpIndex(), op->getThreadId());
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
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay);
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
			addHBEdge(getThreadLastOpIndex(t.getThreadId()),opIndex, op->getThreadId());
		}
		addThreadLastOpIndex(t.getThreadId(), opIndex);
		//		removeThreadLastOpIndex(t->getThreadId());
		aOp = dynamic_cast<Attach_QOperation *>(op);
		t.attachQ(aOp->getQueueId());
		addOperation(opIndex, aOp);
		break;
	case LOOP:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay);
		t.setLooper(true);
		addLoopIndex(t.getThreadId(),opIndex);
//		if(!isCalledFromReplay){
//			addHBEdge(getThreadLastOpIndex(t.getThreadId()),opIndex, op->getThreadId());
//		}
//		removeThreadLastOpIndex(t.getThreadId());
		aOp = dynamic_cast<Attach_QOperation *>(op);
		addOperation(opIndex, aOp);
		break;
	case POST:
	case NATIVE_POST:
	case UI_POST:
	case DELAY_POST:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay);
		pOp = dynamic_cast<PostOperation *>(op);

		//comment the !isCalledFromReplay part of the condition when switching off the HB-optimization.
		if(isDPOR && !isCalledFromReplay){
			addPostHBEdgesInDPOR(pOp->getOpIndex(), pOp->getDestThreadId(), op->getThreadId());
		}
		if(!isCalledFromReplay){
			addHBEdge(getAttachQIndex(pOp->getDestThreadId()), opIndex, op->getThreadId());					// AttachQ ---HB---> Post
		}
		addPostHook(pOp->getPostedTaskId(), opIndex, pOp->getDestThreadId(), pOp->getThreadId());
		updateQueue(pOp->getDestThreadId(), pOp->getPostedTaskId());
		addOperation(opIndex, pOp);
		break;
	case BEGIN:
		t.setTaskId(op->getTaskId());
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay);
		Debug(t.dumpQueue());
		dequedTask = t.deque();
		if(dequedTask!=op->getTaskId()){
			cerr<<" ERROR :: Begin dequeued a task not at the front of queue! dequed task ID :: "<< dequedTask <<endl;
			t.dumpQueue();
			throw error;
		}
//		addLastOpIndex(op->getTaskId(),opIndex);
		//		Debug(cerr<<"BEGIN: looking for task Id : "<<op->getTaskId()<<endl);
		index=getParentPostHookIndex(op->getTaskId());
		//		Debug(cerr<<"index returned is : "<<mapContolLocationToProgramIndex(index)<<endl);
		if(index!=-1){
			pOp = dynamic_cast<PostOperation *>(getOperation(index));
			pOp->setBeginOpIndex(opIndex);
			if(!isCalledFromReplay){
				addHBEdge(index, opIndex, op->getThreadId());										// Post ---HB---> Begin
				addHBEdge(getLoopIndex(op->getThreadId()), opIndex, op->getThreadId());			// Loop ---HB---> Begin
			}
		}
		addOperation(opIndex, op);
		//				Debug(cerr<<" Exiting Begin() "<<endl);
		break;
	case END:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay);
//		if(!isCalledFromReplay){
//			addHBEdge(getLastOpIndex(t.getTaskId()),opIndex, op->getThreadId());
//		}

		index=getParentPostIndex(op->getTaskId());
		if(index!=-1){
			pOp = dynamic_cast<PostOperation *>(getOperation(index));
			pOp->setEndOpIndex(opIndex);
			removePostHook(op->getTaskId());
		}
//		removeLastOpIndex(t.getTaskId());
		t.setTaskId(-1);			//Marking that thread is not running any task now.
		addOperation(opIndex, op);
		break;
	case ENABLE_LIFECYCLE:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay);
		lOp = dynamic_cast<LifecycleOperation *>(op);
		addEnable(make_pair(lOp->getId(),lOp->getState()), opIndex);
		addOperation(opIndex, lOp);
		break;
	case TRIGGER_LIFECYCLE:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay);
		lOp = dynamic_cast<LifecycleOperation *>(op);
		//		Debug(cerr<<"TRIGGER_LIFECYCLE: looking for key:: Id : "<<lOp->getId()<<" State: "<<lOp->getState()<<endl);
		index=getParentEnableIndex(make_pair(lOp->getId(), lOp->getState()));
		//		Debug(cerr<<"index returned is : "<<index<<endl);
		if(index!=-1){
			if(!isCalledFromReplay){
				addHBEdge(index,opIndex, op->getThreadId());
			}
			removeEnable(make_pair(lOp->getId(), lOp->getState()));		//remove enable hook
		}
		addOperation(opIndex, op);
		break;
	case ENABLE_EVENT:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay);
		addOperation(opIndex, op);
		break;
	case TRIGGER_EVENT:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay);
		addOperation(opIndex, op);
		break;
	case ENABLE_WINDOW_FOCUS:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay);
		addOperation(opIndex, op);
		break;
	case TRIGGER_WINDOW_FOCUS:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay);
		addOperation(opIndex, op);
		break;
	case TRIGGER_BROADCAST:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay);
		addOperation(opIndex, op);
		break;
	case TRIGGER_SERVICE:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay);
		addOperation(opIndex, op);
		break;
	case NOP:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay);
		addOperation(opIndex, op);
		break;
	case READ:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay);
		mOp = dynamic_cast<MemoryAccessOperation *>(op);
		addHeapRead(make_pair(mOp->getObjectId(), mOp->getField()), opIndex);
		if(!isCalledFromReplay){
			addHeapWriteHBEdges(make_pair(mOp->getObjectId(), mOp->getField()), opIndex, op->getThreadId());
		}
		addOperation(opIndex, op);
		break;
	case WRITE:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay);
		mOp = dynamic_cast<MemoryAccessOperation *>(op);
		addHeapWrite(make_pair(mOp->getObjectId(), mOp->getField()), opIndex);
		if(!isCalledFromReplay){
			addHeapWriteHBEdges(make_pair(mOp->getObjectId(), mOp->getField()), opIndex, op->getThreadId());
			addHeapReadHBEdges(make_pair(mOp->getObjectId(), mOp->getField()), opIndex, op->getThreadId());
		}
		addOperation(opIndex, op);
		break;
	case READ_STATIC:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay);
		mOp = dynamic_cast<MemoryAccessOperation *>(op);
		addStackRead(make_pair(mOp->getClassName(), mOp->getField()), opIndex);
		if(!isCalledFromReplay){
			addStackWriteHBEdges(make_pair(mOp->getClassName(), mOp->getField()), opIndex, op->getThreadId());
		}
		addOperation(opIndex, op);
		break;
	case WRITE_STATIC:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay);
		mOp = dynamic_cast<MemoryAccessOperation *>(op);
		addStackWrite(make_pair(mOp->getClassName(), mOp->getField()), opIndex);
		if(!isCalledFromReplay){
			addStackWriteHBEdges(make_pair(mOp->getClassName(), mOp->getField()), opIndex, op->getThreadId());
			addStackReadHBEdges(make_pair(mOp->getClassName(), mOp->getField()), opIndex, op->getThreadId());
		}
		addOperation(opIndex, op);
		break;
	case LOCK:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay);
		lockOp = dynamic_cast<LockOperation *>(op);
		addLockHook(lockOp->getObjectId(), lockOp->getOpIndex());
		if(!isCalledFromReplay){
			addUnlockToLockHBEdges(lockOp->getObjectId(), lockOp->getThreadId(), lockOp->getOpIndex());
		}
		addLockOperation(opIndex, lockOp, lockOp->getObjectId(), lockOp->getThreadId());
		addLock(lockOp->getObjectId(), opIndex);

		break;
	case UNLOCK:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay);
		lockOp = dynamic_cast<LockOperation *>(op);
		index = getLockForUnlock(lockOp->getObjectId());
		if(index != -1 && !isCalledFromReplay){
			addHBEdge(index, opIndex, op->getThreadId());
		}
		addUnlockOperation(opIndex, lockOp, lockOp->getObjectId());
		addUnlockHook(lockOp->getObjectId(),opIndex);
		break;
	case WAIT:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay);
		index = popNotifyOperation(op->getThreadId());
		if(index!=-1 && !isCalledFromReplay){
			addHBEdge(index, opIndex, op->getThreadId());
		}
		else if(index==-1){
			cerr<<"ERROR :: Couldn't find a Notify Operation to complete Wait operation at index : "<<opIndex<<endl;
			throw error;
		}
		decrementNotifyCount(op->getThreadId());
		addOperation(opIndex, op);
		break;
	case NOTIFY:
		addSameThreadOrTaskHBEdge(op,isCalledFromReplay);
		nOp = dynamic_cast<NotifyOperation *>(op);
		addNotifyOperation(nOp->getNotifyTagetThreadId(), opIndex);
		incrementNotifyCount(nOp->getNotifyTagetThreadId());
		addOperation(opIndex, op);
		break;
	}
}


//identify nearest dependent operation w.r.t. op to be reordered
int partialProcessOperation(Operation *op, bool isDPOR, int opProgramIndex){		// returns control location of last dependent operation
	Debug(if(op==NULL) cerr<<"NULL passed as operation in fn:processOperation() !!"<<endl);
	int opId = op->getOpId();
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
		index = getLastDependentHeapAccessInstIndexWithRead(make_pair(mOp->getObjectId(), mOp->getField()), opIndex, isDPOR, threadId, opProgramIndex);
		return index;
	case WRITE:
		mOp = dynamic_cast<MemoryAccessOperation *>(op);
		index = getLastDependentHeapAccessInstIndexWithWrite(make_pair(mOp->getObjectId(), mOp->getField()), opIndex, isDPOR, threadId, opProgramIndex);
		return index;
	case READ_STATIC:
		mOp = dynamic_cast<MemoryAccessOperation *>(op);
		index = getLastDependentStackAccessInstIndexWithRead(make_pair(mOp->getClassName(), mOp->getField()), opIndex, isDPOR, threadId, opProgramIndex);
		return index;
	case WRITE_STATIC:
		mOp = dynamic_cast<MemoryAccessOperation *>(op);
		index = getLastDependentStackAccessInstIndexWithWrite(make_pair(mOp->getClassName(), mOp->getField()), opIndex, isDPOR, threadId, opProgramIndex);
		return index;
	case LOCK:
		lockOp = dynamic_cast<LockOperation *>(op);
		index = getLastDependentLockInstIndex(lockOp->getObjectId(), opIndex, threadId, opProgramIndex);
		return index;
	case POST:
	case UI_POST:
	case DELAY_POST:
	case NATIVE_POST:
		if(isDPOR){
			postOp = dynamic_cast<PostOperation *>(op);
			index = getLastPostToSameThreadInstIndex(postOp->getOpIndex(), postOp->getThreadId(), postOp->getDestThreadId(), opProgramIndex);
			return index;
		}
		else return -1;
	default:
		return -1;
	}
}


}

#endif /* OPERATIONPROCESSOR_H_ */
