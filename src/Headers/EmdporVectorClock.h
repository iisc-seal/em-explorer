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

#ifndef EMDPORVECTORCLOCK_H_
#define EMDPORVECTORCLOCK_H_

#include <iterator>
#include <utility>
#include <vector>
#include <map>
#include <set>


#include <BookKeeping.h>
#include <Entities.h>

using namespace std;
using namespace emdpor_bookKeeper;

namespace emdpor_vc{

/* array of VCs stored for each task (field threadClock) and each operation (field indexClock) in the trace.
 * Our implementation of POR algorithms is on the lines of vector clock based implementation of DPOR given
 * in Figure 4 of DPOR paper [Flanagan & Godefroid, POPL'2005]
 */
int **threadClock, **indexClock;
int MAX_THREADS=0, MAX_INDICES=0, MAX_EVENTS=0, CLOCK_WIDTH=0;
int *backupThreadClockVector, *backupIndexClockVector;

// forward declarations
bool if_i_HB_j(int firstIndex, int secondIndex, int firstThread);

void eraseClock(){
	//erase threadClock
	for(int i=0;i<CLOCK_WIDTH;i++)
		for(int j=0;j<CLOCK_WIDTH;j++)
			threadClock[i][j]=0;

	//erase indexClock
	for(int i=0;i<MAX_INDICES;i++)
		for(int j=0;j<CLOCK_WIDTH;j++)
			indexClock[i][j]=0;
}

void initClock(int programLength, int maxThreadId, int maxEventId, bool isDPOR){
	MAX_THREADS=maxThreadId, MAX_INDICES=programLength, MAX_EVENTS=maxEventId;

	/* In case of EM-DPOR which considers different event handlers on the same thread as
	 * different units of concurrency, a vector clock component is assigned to every thread
	 * and every event handler; whereas in case of DPOR where all the event handlers on
	 * the same thread are treated as totally ordered, VC components only correspond to threads
	 * just like pure multi-threaded programs.
	 */
	if(!isDPOR)
		CLOCK_WIDTH = maxThreadId + maxEventId + 1;
	else
		CLOCK_WIDTH = maxThreadId + 1;

	threadClock = new int*[CLOCK_WIDTH];
	indexClock = new int*[programLength];
	backupIndexClockVector = new int[CLOCK_WIDTH];
	backupThreadClockVector = new int[CLOCK_WIDTH];
	for(int i = 0; i < CLOCK_WIDTH; i++)
		threadClock[i] = new int[CLOCK_WIDTH];
	for(int i = 0; i < programLength; i++)
		indexClock[i] = new int[CLOCK_WIDTH];

	eraseClock();
}

void deleteClock(){
	for(int i = 0; i < CLOCK_WIDTH; i++)
	{
		delete[] threadClock[i];
		delete[] indexClock[i];
	}
	delete[] threadClock;
	delete[] indexClock;
	delete[] backupIndexClockVector;
	delete[] backupThreadClockVector;
}

void dumpIndexClock(){
	cerr<<"Dumping Index Clock..."<<endl;
	cerr<<"Index\t";
	for(int i=1; i<CLOCK_WIDTH; ++i)
		cerr<<setw(4)<<i;
	cerr<<endl;
	for(int i = 1; i < MAX_INDICES; ++i){
		cerr<<i<<"\t";
		for(int j = 1; j < CLOCK_WIDTH; ++j){
			cerr<<setw(4)<<indexClock[i][j];
		}
		cerr<<endl;
	}
}

void dumpThreadClock(){
	cerr<<"Dumping Thread Clock..."<<endl;
	cerr<<"Thread\t";
	for(int i=1; i<CLOCK_WIDTH; ++i)
		cerr<<setw(4)<<i;
	cerr<<endl;
	for(int i = 1; i < CLOCK_WIDTH; ++i){
		cerr<<i<<"\t";
		for(int j = 1; j < CLOCK_WIDTH; ++j){
			cerr<<setw(4)<<threadClock[i][j];
		}
		cerr<<endl;
	}
}

void dumpClock(){
	DumpClock(dumpThreadClock());
	DumpClock(dumpIndexClock());
}

inline int getClockIndexForEvent(int eventId){
	return MAX_THREADS + eventId;
}

inline int getEventIdForClockIndex(int clockIndex){
	if(clockIndex <= MAX_THREADS)
		return -1;
	return clockIndex - MAX_THREADS;
}

vector<int> getDependentOpIndices(Operation *op){
	int optype = op->getOpType();
	vector<int> resultVector;
	vector<int> readIndices;
	vector<int> writeIndices;
	vector<int> lockIndices;
	vector < pair<int,int> > postVector;
	MemoryAccessOperation * mOp;
	LockOperation *lockOp;
	PostOperation *postOp;
	pair<int, int> aPair;
	int index;
	switch(optype){
	case READ:
		mOp = dynamic_cast<MemoryAccessOperation *>(op);
		writeIndices = getHeapWriteIndices(make_pair(mOp->getObjectId(), mOp->getField()));
		return writeIndices;
		break;
	case WRITE:
		mOp = dynamic_cast<MemoryAccessOperation *>(op);
		readIndices = getHeapReadIndices(make_pair(mOp->getObjectId(), mOp->getField()));
		writeIndices = getHeapWriteIndices(make_pair(mOp->getObjectId(), mOp->getField()));
		for(vector<int>::iterator it = writeIndices.begin(); it != writeIndices.end(); ++it){
			readIndices.push_back(*it);
		}
		return readIndices;
		break;
	case READ_STATIC:
		mOp = dynamic_cast<MemoryAccessOperation *>(op);
		writeIndices = getStackWriteIndices(make_pair(mOp->getClassName(), mOp->getField()));
		return writeIndices;
		break;
	case WRITE_STATIC:
		mOp = dynamic_cast<MemoryAccessOperation *>(op);
		readIndices = getStackReadIndices(make_pair(mOp->getClassName(), mOp->getField()));
		writeIndices = getStackWriteIndices(make_pair(mOp->getClassName(), mOp->getField()));
		for(vector<int>::iterator it = writeIndices.begin(); it != writeIndices.end(); ++it){
			readIndices.push_back(*it);
		}
		return readIndices;
		break;
	case LOCK:
		lockOp = dynamic_cast<LockOperation *>(op);
		lockIndices = getLocksOnId(lockOp->getObjectId());
		return lockIndices;
		break;
	case POST:
	case UI_POST:
	case DELAY_POST:
	case NATIVE_POST:
		postOp = dynamic_cast<PostOperation *>(op);
		postVector = getPostsToThread(postOp->getDestThreadId());
		for(vector< pair<int,int> >::iterator it = postVector.begin(); it != postVector.end(); ++it){
			aPair = *it;
			index = aPair.first;
			resultVector.push_back(index);
		}
		return resultVector;
		break;
	}
	return resultVector;
}

void getPointWiseMaxVector(int inOutVector1[], const int inVector2[]){
//	Debug(cerr<<"CLOCK_WIDTH : "<<CLOCK_WIDTH<<endl);
	for(int i =0; i<CLOCK_WIDTH; i++){
//		Debug(cerr<<"i : "<<i<<endl);
		inOutVector1[i] = (inOutVector1[i] > inVector2[i]) ? inOutVector1[i] : inVector2[i];
	}
}

void copyVector(int inOutVector1[], const int inVector2[]){
	for(int i =0; i<CLOCK_WIDTH; i++){
		inOutVector1[i] = inVector2[i];
	}
}

// Do not replicate this change in DPOR configuration, because there clock vector doesn't have columns for tasks.
// See initClock() function for details.
inline int getTaskForOp(Operation *op_j){
	int task_j;
	if(op_j->getTaskId() == -1 /*|| isDPOR*/)
		task_j = op_j->getThreadId();
	else
		task_j = getClockIndexForEvent(op_j->getTaskId());
	return task_j;
}

inline void printDporState(bool isDPOR){
	if(isDPOR)
		cerr<<"isDPOR = true"<<endl;
	else
		cerr<<"isDPOR = false"<<endl;
}


void addHBEdge(Operation *op_i, Operation *op_j, bool isDPOR){
	if(op_i == NULL){
		cerr<<"ERROR::fn:addHBEdge() - op_i is null"<<endl; throw error;
	}
	if(op_j == NULL){
		cerr<<"ERROR::fn:addHBEdge() - op_j is null"<<endl; throw error;
	}
	Debug(cerr<<"addHBEdge() called !"<<endl);
	Debug(printDporState(isDPOR));
	Debug(cerr<<"Source op : "<<op_i->getOpType()<<endl);
	Debug(cerr<<"Destination op : "<<op_j->getOpType()<<endl);

	int clockVector[CLOCK_WIDTH];
    int i = op_i->getOpIndex();
	int task_j = getTaskForOp(op_j);
    int destIndex = op_j->getOpIndex();

	copyVector(clockVector, indexClock[i]);
	clockVector[task_j] = destIndex;
	getPointWiseMaxVector(threadClock[task_j], clockVector);
	getPointWiseMaxVector(indexClock[destIndex], clockVector);
}

int temp_task_j=-1, temp_destIndex=-1;
bool restoreRequired = false;

void backup(int task_j, int destIndex){
	Debug(cerr<<"Begin:: fn- backup()"<<endl);
	temp_task_j = task_j;
	temp_destIndex = destIndex;
	copyVector(backupThreadClockVector, threadClock[task_j]);
	copyVector(backupIndexClockVector, indexClock[destIndex]);
	restoreRequired = true;
	Debug(cerr<<"End:: fn- backup()"<<endl);
}

void addTempHBEdge(Operation *op_i, Operation *op_j, bool isDPOR){
	Debug(cerr<<"Begin:: fn- addTempHBEdge()"<<endl);
	if(op_i == NULL){
		cerr<<"ERROR::fn:addHBEdge() - op_i is null"<<endl; throw error;}
	if(op_j == NULL){
		cerr<<"ERROR::fn:addHBEdge() - op_j is null"<<endl; throw error;}
	Debug(printDporState(isDPOR));
	Debug(cerr<<"Source op : "<<op_i->getOpIndex()<<endl);
	Debug(cerr<<"Destination op : "<<op_j->getOpIndex()<<endl);
	if( if_i_HB_j(op_i->getOpIndex(), op_j->getOpIndex(), op_i->getThreadId()) ){
		Debug(cerr<<"Source & destination already have HB Edge, returning!"<<endl);
		return;
	}
	int clockVector[CLOCK_WIDTH];
	int i = op_i->getOpIndex();
	int task_j = getTaskForOp(op_j);
	int destIndex = op_j->getOpIndex();

	backup(task_j, destIndex);

	copyVector(clockVector, indexClock[i]);
	clockVector[task_j] = destIndex;
	getPointWiseMaxVector(threadClock[task_j], clockVector);
	getPointWiseMaxVector(indexClock[destIndex], clockVector);
	Debug(cerr<<"End:: fn- addTempHBEdge()"<<endl);
}

void removeTempHBEdge(){
	Debug(cerr<<"Begin:: fn- removeTempHBEdge()"<<endl);
	if(!restoreRequired)
	{Debug(cerr<<"returning without any backup!"<<endl); return;}
	if(temp_task_j==-1 || temp_destIndex==-1){
		cerr<<"ERROR::trying to add an invalid HB Edge with either source or destination or both values = -1"<<endl;
		throw error;
	}

	copyVector(threadClock[temp_task_j], backupThreadClockVector);
	copyVector(indexClock[temp_destIndex], backupIndexClockVector);

	restoreRequired = false;
	Debug(cerr<<"End:: fn- removeTempHBEdge()"<<endl);
}

// it also initializes posted task's component in post's vector clock.
void addAttachqPostHBEdge(Operation *op_i, PostOperation *op_j, bool isDPOR){
	int clockVector[CLOCK_WIDTH];
	int i = op_i->getOpIndex();
	int task_j = getTaskForOp(op_j);
	int destIndex = op_j->getOpIndex();

	int postedTaskClockIndex = getClockIndexForEvent(op_j->getPostedTaskId());
//	cerr<<"postedTaskClockIndex : "<<postedTaskClockIndex<<endl;
	copyVector(clockVector, indexClock[i]);
	clockVector[task_j] = destIndex;
	clockVector[postedTaskClockIndex] = 1;
	getPointWiseMaxVector(threadClock[task_j], clockVector);
	getPointWiseMaxVector(indexClock[destIndex], clockVector);

	// adding post to begin HB here
	Debug(dumpClock());
	Debug(cerr<<"initializing posted event clock"<<endl);
	Debug(cerr<<"postedTaskClockIndex : "<<postedTaskClockIndex<<" destIndex : "<<destIndex<<endl);
	getPointWiseMaxVector(threadClock[postedTaskClockIndex], indexClock[destIndex]);
	Debug(dumpClock());
}

void addHeapReadHBEdges(pair<long,int> key, Operation *op, bool isDPOR){
	vector<int> readIndices = getHeapReadIndices(key);
	for(vector<int>::iterator it = readIndices.begin(); it != readIndices.end(); ++it)
		addHBEdge(getOperation(*it), op, isDPOR);
}

void addHeapWriteHBEdges(pair<long,int> key, Operation *op, bool isDPOR){
	vector<int> writeIndices = getHeapWriteIndices(key);
	for(vector<int>::iterator it = writeIndices.begin(); it != writeIndices.end(); ++it){
		Debug(cerr<<"write index = "<<*it<<endl);
		addHBEdge(getOperation(*it), op, isDPOR);
	}
}

void addStackReadHBEdges(pair<string,int> key, Operation *op, bool isDPOR){
	vector<int> readIndices = getStackReadIndices(key);
	for(vector<int>::iterator it = readIndices.begin(); it != readIndices.end(); ++it)
		addHBEdge(getOperation(*it), op, isDPOR);
}

void addStackWriteHBEdges(pair<string,int> key, Operation *op, bool isDPOR){
	vector<int> writeIndices = getStackWriteIndices(key);
	for(vector<int>::iterator it = writeIndices.begin(); it != writeIndices.end(); ++it){
		addHBEdge(getOperation(*it), op, isDPOR);
	}
}

void addPostHBEdgesInDPOR(PostOperation *pOp, bool isDPOR){
	vector< pair<int,int> > dependentPosts = getPostsToThread(pOp->getDestThreadId());
	for(int i=0; i<(int)dependentPosts.size(); ++i){
		pair<int,int> aPair = dependentPosts.at(i);
		int depPostIndex = aPair.first;
		addHBEdge(getOperation(depPostIndex), pOp, isDPOR);
	}
}

void addFifoHBEdges(Operation *op_j, bool isDPOR){

	Debug(cerr<<"fn:addFifoHBEdges()-begin"<<endl);
	Debug(printDporState(isDPOR));

	vector< pair<int,int> > dependentPosts = getPostsToThread(op_j->getThreadId());
	int task_j = getTaskForOp(op_j), destIndex=op_j->getOpIndex();
	int clockVector[CLOCK_WIDTH];
	copyVector(clockVector, indexClock[destIndex]);

	for(int i=0; i<(int)dependentPosts.size(); ++i){
		pair<int,int> aPair = dependentPosts.at(i);
		int depPostIndex = aPair.first;
		Operation *op = getOperation(depPostIndex);
		PostOperation *pOp = dynamic_cast<PostOperation *>(op);
		Debug(cerr<<"dependent post id : "<<pOp->getOpIndex()<<endl);

		/* the below condition is a way of checking if pOp happens before post of task_j.
		 * Since we dont have delay posts (these have been modelled as posts with 0
		 * delay posted by another thread non-deterministically) or posts to front of queue,
		 * happens before between post ops posting to same thread imply that their
		 * corresponding handlers are ordered by FIFO HB rule presented in various Android concurrency papers.
		 */
		if( threadClock[task_j][getClockIndexForEvent(pOp->getPostedTaskId())] >=1){
			Debug(dumpClock());
			Debug(cerr<<"taking join with event id : "<<pOp->getPostedTaskId()<<endl);
			getPointWiseMaxVector(clockVector, threadClock[getClockIndexForEvent(pOp->getPostedTaskId())]);

			Debug(dumpClock());
		}
	}
	getPointWiseMaxVector(threadClock[task_j], clockVector);
	getPointWiseMaxVector(indexClock[destIndex], clockVector);
	Debug(dumpClock());
	Debug(cerr<<"fn:addFifoHBEdges()-end"<<endl);
}

void addUnlockToLockHBEdges(int objectId, int threadId, Operation *lockOp, bool isDPOR){
	map<long, vector<int> >::iterator it = unlockMap.find(objectId);
	Debug(cerr<<"fn:addUnlockToLockHBEdges() : begin"<<endl);
	if(it!=unlockMap.end()){
		vector<int> unlockVector = it->second;
		for(vector<int>::iterator vit = unlockVector.begin(); vit != unlockVector.end(); ++vit){
			int unlockIndex = *vit;
			Operation *op = getOperation(unlockIndex);
			if(op->getThreadId() != threadId){
				Debug(cerr<<"adding HB Edge from "<<unlockIndex<<" to "<< lockOp->getOpIndex()<<endl);
				addHBEdge(op, lockOp, isDPOR);
			}
		}
	}
	Debug(cerr<<"fn:addUnlockToLockHBEdges() : end"<<endl);
}

bool if_i_HB_j(int firstIndex, int secondIndex, int firstThread){
	if(firstIndex <= indexClock[secondIndex][firstThread])
		return true;
	return false;
}

bool if_i_HB_p(int firstIndex, int processId, int firstThread){
	DebugSelective(cerr<<"firstIndex :"<<firstIndex<<" threadClock["<<processId<<"]["<<firstThread<<"] = "<<threadClock[processId][firstThread]<<endl);
	if(firstIndex <= threadClock[processId][firstThread]){
		DebugSelective(cerr<<"returning true"<<endl);
		return true;
	}
	DebugSelective(cerr<<"returning false"<<endl);
	return false;
}

bool if_i_HB_task(Operation *op_i, Operation *op_j, bool isDPOR){

	DebugSelective(cerr<<"fn:if_i_HB_task() : dumping arguments - "<<endl);
	Debug(printDporState(isDPOR));

	DebugSelective(cerr<<"Source op : "<<op_i->getOpType()<<endl);
	DebugSelective(cerr<<"Destination op : "<<op_j->getOpType()<<endl);

	int i = op_i->getOpIndex();
	int task_j = getTaskForOp(op_j), task_i = getTaskForOp(op_i);

	DebugSelective(cerr<<"i :"<<i<<" threadClock["<<task_j<<"]["<<task_i<<"] = "<<threadClock[task_j][task_i]<<endl);
	if(i <= threadClock[task_j][task_i]){
		return true;
	}

	return false;
}


/******** functions required for getting last dependent operation. ********/
/* These functions correspond to lines looking for nearest dependent ops in the Explore Algorithm given in paper */

/* This function returns all dependent instructions with write operation opJ until the first dependent
 * and reorderable nearest write.
 *
 * Our implementation of finding all the dependent operations to be reordered is not quite optimized.
 * Currently the implementation does the following:
 * Given a write operation opJ it returns all the nearest reorderable and dependent reads upto
 * and including the nearest reorderable and dependent write.
 *
 * An optimized implementation would do the following:
 * Firstly, let d (read or write) such that d does not HB opJ be the nearest dependent operation to
 * be reordered w.r.t. opJ. If d is not executed within an event handler or if d is a write then it is
 * sufficient to  reorder opJ only w.r.t. d and we do not have to identify a set of nearest dependent
 * ops to be reordered.
 * However, if d is a read executed within an event handler on a thread different from opJ then
 * we need to identify a set D of dependent and reorderable read ops on same thread as d till we either come across
 * a read on another thread or come across a conflicting write operation.
 * The set D does not have to include the read from the other thread or the nearest write.
 * Then, it is sufficient if this function returns a set {d} U D.
 *
 * The intuitive reasoning for an optimization in this line is presented in the extended version of the paper
 * which contains various optimizations to the algorithm.
 *
 */
set<int> getAllDependentHeapAccessInstIndexWithWrite(pair<long,int> key, int writeAccessIndex, bool isDPOR, int threadId, int opProgramIndex, Operation *opJ){
	Debug(cerr<<"fn: getAllDependentHeapAccessInstIndexWithWrite() :: begin"<<endl);
	vector<int> readIndices = getHeapReadIndices(key);
	vector<int> writeIndices = getHeapWriteIndices(key);
	int lastWrite=-1;
	set<int> resultSet;
	if(writeIndices.empty() && readIndices.empty()){
		Debug(cerr<<"case :: both readIndices.empty() && writeIndices.empty()"<<endl);
	}

	if(!writeIndices.empty()){
		Debug(cerr<<"case :: !writeIndices.empty()"<<endl);
		for(vector<int>::reverse_iterator it = writeIndices.rbegin(); it != writeIndices.rend(); ++it){
			int i = *it;
			Operation *op = getOperation(i);
			if(op!=NULL){
				if( !if_i_HB_task(op,opJ,isDPOR) ){
					lastWrite=i;
					resultSet.insert(i);
					break;
				}
			}
			else{
				cerr<<"ERROR:: in fn:getAllDependentHeapAccessInstIndexWithWrite(), operation found to be NULL"<<endl;
				throw error;
			}
		}
	}

	if(!readIndices.empty()){
		Debug(cerr<<"case :: !readIndices.empty()"<<endl);
		for(vector<int>::reverse_iterator it = readIndices.rbegin(); it != readIndices.rend(); ++it){
			int i = *it;
			Operation *op = getOperation(i);
			if(op!=NULL){
				if( !if_i_HB_task(op,opJ,isDPOR) ){

					if(i < lastWrite)
						continue;

					resultSet.insert(i);
				}
			}
			else{
				cerr<<"ERROR:: in fn:getAllDependentHeapAccessInstIndexWithWrite(), operation found to be NULL"<<endl;
				throw error;
			}
		}
	}

	return resultSet;
}

int getLastDependentHeapAccessInstIndexWithWrite(pair<long,int> key, int writeAccessIndex, bool isDPOR, int threadId, int opProgramIndex, Operation *opJ){
	Debug(cerr<<"fn: getLastDependentHeapAccessInstIndexWithWrite() :: begin"<<endl);
	vector<int> readIndices = getHeapReadIndices(key);
	vector<int> writeIndices = getHeapWriteIndices(key);
	int lastRead=-1, lastWrite=-1;
	if(!readIndices.empty() && !writeIndices.empty()){
		Debug(cerr<<"case :: !readIndices.empty() && !writeIndices.empty()"<<endl);
		for(vector<int>::reverse_iterator it = readIndices.rbegin(); it != readIndices.rend(); ++it){
			int i = *it;
			int lastThreadId=-1;
			Operation *op = getOperation(i);
			if(op!=NULL){
				lastThreadId = op->getThreadId();
				if( !if_i_HB_task(op,opJ,isDPOR) ){
					if(isDPOR){
						if(threadId == lastThreadId)			// not considering two operations on the same thread as dependent.
							continue;
					}
					lastRead=i;
					break;
				}
			}
			else{
				cerr<<"ERROR:: in fn:getLastDependentHeapAccessInstIndexWithWrite(), operation found to be NULL"<<endl;
				throw error;
			}
		}

		for(vector<int>::reverse_iterator it = writeIndices.rbegin(); it != writeIndices.rend(); ++it){
			int i = *it;
			int lastThreadId=-1;
			Operation *op = getOperation(i);
			if(op!=NULL){
				lastThreadId = op->getThreadId();
				if( !if_i_HB_task(op,opJ,isDPOR) ){
					if(isDPOR){
						if(threadId == lastThreadId)			// not considering two operations on the same thread as dependent.
							continue;
					}
					lastWrite=i;
					break;
				}
			}
			else{
				cerr<<"ERROR:: in fn:getLastDependentHeapAccessInstIndexWithWrite(), operation found to be NULL"<<endl;
				throw error;
			}
		}
		Debug(cerr<<"lastRead = "<<lastRead<<" lastWrite = "<<lastWrite<<endl);
		if(lastRead > lastWrite)
			return lastRead;
		return lastWrite;
	}
	if(!readIndices.empty()){
		Debug(cerr<<"case :: !readIndices.empty()"<<endl);
		for(vector<int>::reverse_iterator it = readIndices.rbegin(); it != readIndices.rend(); ++it){
			int i = *it;
			Operation *op = getOperation(i);
			int lastThreadId=-1;
			if(op!=NULL){
				lastThreadId = op->getThreadId();
				if( !if_i_HB_task(op,opJ,isDPOR) ){
					if(isDPOR){
						if(threadId == lastThreadId)			// not considering two operations on the same thread as dependent.
							continue;
					}
					lastRead=i;
					return lastRead;;
				}
			}
			else{
				cerr<<"ERROR:: in fn:getLastDependentHeapAccessInstIndexWithWrite(), operation found to be NULL"<<endl;
				throw error;
			}
		}
	}

	if(!writeIndices.empty()){
		Debug(cerr<<"case :: !writeIndices.empty()"<<endl);
		for(vector<int>::reverse_iterator it = writeIndices.rbegin(); it != writeIndices.rend(); ++it){
			int i = *it;
			int lastThreadId=-1;
			Operation *op = getOperation(i);
			if(op!=NULL){
				lastThreadId = op->getThreadId();
				if( !if_i_HB_task(op,opJ,isDPOR) ){
					if(isDPOR){
						if(threadId == lastThreadId)			// not considering two operations on the same thread as dependent.
							continue;
					}
					lastWrite=i;
					return lastWrite;
				}
			}
			else{
				cerr<<"ERROR:: in fn:getLastDependentHeapAccessInstIndexWithWrite(), operation found to be NULL"<<endl;
				throw error;
			}
		}
	}

	if(writeIndices.empty() && readIndices.empty()){
		Debug(cerr<<"case :: both readIndices.empty() && writeIndices.empty()"<<endl);
	}

	return -1;
}

int getLastDependentHeapAccessInstIndexWithRead(pair<long,int> key, int readAccessIndex, bool isDPOR, int threadId, int opProgramIndex, Operation *opJ){
	//	dumpHBGraph();
	vector<int> writeIndices = getHeapWriteIndices(key);

	if(!writeIndices.empty()){
		int lastWrite=-1;
		for(vector<int>::reverse_iterator it = writeIndices.rbegin(); it != writeIndices.rend(); ++it){
			int i = *it;
			int lastThreadId=-1;
			Operation *op = getOperation(i);
			if(op!=NULL){
				lastThreadId = op->getThreadId();
				if( !if_i_HB_task(op,opJ,isDPOR) ){
					if(isDPOR){
						if(threadId == lastThreadId)			// not considering two operations on the same thread as dependent.
							continue;
					}
					lastWrite=i;
					return lastWrite;
				}
			}
			else{
				cerr<<"ERROR:: in fn:getLastDependentHeapAccessInstIndexWithRead(), operation found to be NULL"<<endl;
				throw error;
			}
		}
	}
	return -1;
}


/* This function returns all dependent instructions with write operation opJ until the first dependent
 * and reorderable nearest write.
 *
 * Our implementation of finding all the dependent operations to be reordered is not quite optimized.
 * Currently the implementation does the following:
 * Given a write operation opJ it returns all the nearest reorderable and dependent reads upto
 * and including the nearest reorderable and dependent write.
 *
 * An optimized implementation would do the following:
 * Firstly, let d (read or write) such that d does not HB opJ be the nearest dependent operation to
 * be reordered w.r.t. opJ. If d is not executed within an event handler or if d is a write then it is
 * sufficient to  reorder opJ only w.r.t. d and we do not have to identify a set of nearest dependent
 * ops to be reordered.
 * However, if d is a read executed within an event handler on a thread different from opJ then
 * we need to identify a set D of dependent and reorderable read ops on same thread as d till we either come across
 * a read on another thread or come across a conflicting write operation.
 * The set D does not have to include the read from the other thread or the nearest write.
 * Then, it is sufficient if this function returns a set {d} U D.
 *
 * The intuitive reasoning for an optimization in this line is presented in the extended version of the paper
 * which contains various optimizations to the algorithm.
 *
 */
set<int> getAllDependentStackAccessInstIndexWithWrite(pair<string,int> key, int writeAccessIndex, bool isDPOR, int threadId, int opProgramIndex, Operation *opJ){
	Debug(cerr<<"fn: getAllDependentHeapAccessInstIndexWithWrite() :: begin"<<endl);
	vector<int> readIndices = getStackReadIndices(key);
	vector<int> writeIndices = getStackWriteIndices(key);
	int lastWrite=-1;
	set<int> resultSet;
	if(writeIndices.empty() && readIndices.empty()){
		Debug(cerr<<"case :: both readIndices.empty() && writeIndices.empty()"<<endl);
	}

	if(!writeIndices.empty()){
		Debug(cerr<<"case :: !writeIndices.empty()"<<endl);
		for(vector<int>::reverse_iterator it = writeIndices.rbegin(); it != writeIndices.rend(); ++it){
			int i = *it;
			Operation *op = getOperation(i);
			if(op!=NULL){
				if( !if_i_HB_task(op,opJ,isDPOR) ){
					lastWrite=i;
					resultSet.insert(i);
					break;
				}
			}
			else{
				cerr<<"ERROR:: in fn:getAllDependentStackAccessInstIndexWithWrite(), operation found to be NULL"<<endl;
				throw error;
			}
		}
	}

	if(!readIndices.empty()){
		Debug(cerr<<"case :: !readIndices.empty()"<<endl);
		for(vector<int>::reverse_iterator it = readIndices.rbegin(); it != readIndices.rend(); ++it){
			int i = *it;
			Operation *op = getOperation(i);
			if(op!=NULL){
				if( !if_i_HB_task(op,opJ,isDPOR) ){

					if(i < lastWrite)
						continue;

					resultSet.insert(i);
				}
			}
			else{
				cerr<<"ERROR:: in fn:getAllDependentStackAccessInstIndexWithWrite(), operation found to be NULL"<<endl;
				throw error;
			}
		}
	}

	return resultSet;
}


int getLastDependentStackAccessInstIndexWithWrite(pair<string,int> key, int writeAccessIndex, bool isDPOR, int threadId, int opProgramIndex, Operation *opJ){

	vector<int> readIndices = getStackReadIndices(key);
	vector<int> writeIndices = getStackWriteIndices(key);

//	int j = writeAccessIndex;
	int lastRead=-1, lastWrite=-1;
	if(!readIndices.empty() && !writeIndices.empty()){
		Debug(cerr<<"case :: !readIndices.empty() && !writeIndices.empty()"<<endl);
		for(vector<int>::reverse_iterator it = readIndices.rbegin(); it != readIndices.rend(); ++it){
			int i = *it;
			int lastThreadId=-1;
			Operation *op = getOperation(i);
			if(op!=NULL){
				lastThreadId = op->getThreadId();
				if( !if_i_HB_task(op,opJ,isDPOR) ){
					if(isDPOR){
						if(threadId == lastThreadId)			// not considering two operations on the same thread as dependent.
							continue;
					}
					lastRead=i;
					break;
				}
			}
			else{
				cerr<<"ERROR:: in fn:getLastDependentStackAccessInstIndexWithWrite(), operation found to be NULL"<<endl;
				throw error;
			}
		}

		for(vector<int>::reverse_iterator it = writeIndices.rbegin(); it != writeIndices.rend(); ++it){
			int i = *it;
			int lastThreadId=-1;
			Operation *op = getOperation(i);
			if(op!=NULL){
				lastThreadId = op->getThreadId();
				if( !if_i_HB_task(op,opJ,isDPOR) ){
					if(isDPOR){
						if(threadId == lastThreadId)			// not considering two operations on the same thread as dependent.
							continue;
					}
					lastWrite=i;
					break;
				}
			}
			else{
				cerr<<"ERROR:: in fn:getLastDependentStackAccessInstIndexWithWrite(), operation found to be NULL"<<endl;
				throw error;
			}
		}
		Debug(cerr<<"lastRead = "<<lastRead<<" lastWrite = "<<lastWrite<<endl);
		if(lastRead > lastWrite)
			return lastRead;
		return lastWrite;
	}
	if(!readIndices.empty()){
		Debug(cerr<<"case :: !readIndices.empty()"<<endl);
		for(vector<int>::reverse_iterator it = readIndices.rbegin(); it != readIndices.rend(); ++it){
			int i = *it;
			Operation *op = getOperation(i);
			int lastThreadId=-1;
			if(op!=NULL){
				lastThreadId = op->getThreadId();
				if( !if_i_HB_task(op,opJ,isDPOR) ){
					if(isDPOR){
						if(threadId == lastThreadId)			// not considering two operations on the same thread as dependent.
							continue;
					}
					lastRead=i;
					return lastRead;
				}
			}
			else{
				cerr<<"ERROR:: in fn:getLastDependentStackAccessInstIndexWithWrite(), operation found to be NULL"<<endl;
				throw error;
			}
		}
	}

	if(!writeIndices.empty()){
		Debug(cerr<<"case :: !writeIndices.empty()"<<endl);
		for(vector<int>::reverse_iterator it = writeIndices.rbegin(); it != writeIndices.rend(); ++it){
			int i = *it;
			int lastThreadId=-1;
			Operation *op = getOperation(i);
			if(op!=NULL){
				lastThreadId = op->getThreadId();
				if( !if_i_HB_task(op,opJ,isDPOR) ){
					if(isDPOR){
						if(threadId == lastThreadId)			// not considering two operations on the same thread as dependent.
							continue;
					}
					lastWrite=i;
					return lastWrite;
				}
			}
			else{
				cerr<<"ERROR:: in fn:getLastDependentStackAccessInstIndexWithWrite(), operation found to be NULL"<<endl;
				throw error;
			}
		}
	}

	if(writeIndices.empty() && readIndices.empty()){
		Debug(cerr<<"case :: both readIndices.empty() && writeIndices.empty()"<<endl);
	}

	return -1;
}

// in case of isDPOR is true, don't return an index which is on same threadId
int getLastDependentStackAccessInstIndexWithRead(pair<string,int> key, int readAccessIndex, bool isDPOR, int threadId, int opProgramIndex, Operation *opJ){
	//dumpHBGraph();
	vector<int> writeIndices = getStackWriteIndices(key);
//	int j = readAccessIndex;
	if(!writeIndices.empty()){
		int lastWrite=-1;
		for(vector<int>::reverse_iterator it = writeIndices.rbegin(); it != writeIndices.rend(); ++it){
			int i = *it;
			int lastThreadId=-1;
			Operation *op = getOperation(i);
			if(op!=NULL){
				lastThreadId = op->getThreadId();
				if( !if_i_HB_task(op,opJ,isDPOR) ){
					if(isDPOR){
						if(threadId == lastThreadId)			// not considering two operations on the same thread as dependent.
							continue;
					}
					lastWrite=i;
					return lastWrite;
				}
			}
			else{
				cerr<<"ERROR:: in fn:getLastDependentStackAccessInstIndexWithRead(), operation found to be NULL"<<endl;
				throw error;
			}
		}
	}
	return -1;
}

// do not return an index which is on same threadId as we are not considering two lock ops on same thread as dependent.
int getLastDependentLockInstIndex(long objId, int threadId, bool isDPOR, Operation *opJ){
	vector<int> lockIndices = getLocksOnId(objId);
//	int j = lockIndex;
	if(!lockIndices.empty()){
		for(vector<int>::reverse_iterator it = lockIndices.rbegin(); it != lockIndices.rend(); ++it){
			int i = *it;
			int lastThreadId=-1;
			Operation *op = getOperation(i);
			if(op!=NULL){
				lastThreadId = op->getThreadId();
				if( !if_i_HB_task(op,opJ,isDPOR) ){
					if(threadId == lastThreadId)			// not considering two operations on the same thread as dependent.
						continue;
					int lastLock=i;
					return lastLock;
				}
			}
			else{
				cerr<<"ERROR:: in fn:getLastDependentLockInstIndex(), operation found to be NULL"<<endl;
				throw error;
			}
		}
	}
	return -1;
}

/* This function returns all dependent instructions with lock operation opJ until.
 *
 * A scope for optimization is mentioned as a detailed comment in the function body.
 */
set<int> getAllDependentLockInstOnAThreadUntilDiffThreadLockInterference(long objId, int threadId, bool isDPOR, Operation *opJ){
	set<int> resultSet;
	vector<int> lockIndices = getLocksOnId(objId);
	if(!lockIndices.empty()){
		int candidateThreadId = -1;
		for(vector<int>::reverse_iterator it = lockIndices.rbegin(); it != lockIndices.rend(); ++it){
			int i = *it;
			int lastThreadId=-1;
			Operation *op = getOperation(i);
			if(op!=NULL){
				lastThreadId = op->getThreadId();
				if( !if_i_HB_task(op,opJ,isDPOR) ){
					if(threadId == lastThreadId)			// not considering two operations on the same thread as dependent.
						continue;

					if(candidateThreadId == -1)				// take 1st dependent and reorderable lockOp-thread as candidate thread.
						candidateThreadId = lastThreadId;

					if(candidateThreadId == lastThreadId)	// take all dependent locks on the candidate thread in result set...
						resultSet.insert(i);
					else									// ...until a dependent lock from different thread is encountered
						break;
				}
			}
			else{
				cerr<<"ERROR:: in fn:getAllDependentLockInstOnAThreadUntilDiffThreadLockInterference(), operation found to be NULL"<<endl;
				throw error;
			}
		}
	}
	/* The above implementation identifies a superset of locks on the same thread for reordering w.r.t.
	 * lock op in opJ than required. It is sufficient to identify a set of locks instead of a single nearest lock only
	 * if the nearest reorderable lock op is executed within an event handler. But we dont make that
	 * distinction in implementation, which is alright. This will only cost some redundant effort.
	 */
	return resultSet;
}


int getLastPostToSameThreadInstIndex(int postIndex, int threadId, int destinationThreadId, int postProgramIndex, bool isDPOR, Operation *opJ){
	//	dumpHBGraph();
	vector < pair<int,int> > postVector = getPostsToThread(destinationThreadId);
	if(!postVector.empty()){
		for(vector< pair<int,int> >::reverse_iterator it = postVector.rbegin(); it != postVector.rend(); ++it){
			pair<int, int> aPair = *it;
			int i = aPair.first;
			int lastThreadId=-1;
			Operation *op = getOperation(i);
			if(op!=NULL){
				lastThreadId = op->getThreadId();
				if( !if_i_HB_task(op,opJ,isDPOR) ){
					if(threadId == lastThreadId)			// not considering two operations on the same thread as dependent.
						continue;
					int lastPost = i;
					return lastPost;
				}
			}
			else{
				cerr<<"ERROR:: in fn:getLastPostToSameThreadInstIndex(), operation found to be NULL"<<endl;
				throw error;
			}
		}
	}
	return -1;
}

void addExplicitHBEdges(int target, int targetThreadId, bool isDPOR){
	set<int> sourceSet =  explicitDependenciesBackup.getSources(target);
	if(sourceSet.empty())
		return;
	int targetCL = mapProgramIndexToControlLocation(target);
	for(set<int>::iterator it = sourceSet.begin(); it != sourceSet.end(); ++it){
		int sourceCL = mapProgramIndexToControlLocation(*it);
		Debug(cerr<<"Adding explicit HBEdge"<<endl);
		addHBEdge(getOperation(sourceCL), getOperation(targetCL), isDPOR);
	}
}

// Helper to compute backtracking choices in DPOR-style
set<int> findTaskExecutableHappenedBeforeThread(int stateIndex, Operation *opJ, int threadIdOfOpI, bool isDPOR){
	set<int> resultSet;
	int j = opJ->getOpIndex();
	int i = stateIndex;
	Debug(cerr<<" j = "<<j<<" i = "<<i<<endl);
	for(int k = j-1; k > i; k--){
		Operation *op = getOperation(k);
		if( if_i_HB_task(op,opJ,isDPOR) ){
			Debug(cerr<<" k :: "<<k<<endl);
			Thread threadState = getThreadState(stateIndex, op->getThreadId());
			if(op->getThreadId() == threadIdOfOpI)
				continue;
			if(isThreadEnabled(stateIndex, op->getThreadId(), threadState)){
				if(!isThreadLooping(threadState)){
					resultSet.insert(op->getThreadId());
				}
				else if(isTaskExecutable(op->getTaskId(), threadState)){
					resultSet.insert(op->getThreadId());
				}
			}
		}
	}
	return resultSet;			// signifies no such thread found!
}
/*
set<int> findHappenedBeforeEnabledThreads(int stateIndex, Operation *opJ, int threadIdOfOpI, bool isDPOR){
	set<int> resultSet;
	int j = opJ->getOpIndex();
	int i = stateIndex;
	Debug(cerr<<" j = "<<j<<" i = "<<i<<endl);
	for(int k = j-1; k > i; k--){
		Operation *op = getOperation(k);
		if( if_i_HB_task(op,opJ,isDPOR) ){
			Debug(cerr<<" k :: "<<k<<endl);
			Thread threadState = getThreadState(stateIndex, op->getThreadId());
			if(op->getThreadId() == threadIdOfOpI)
				continue;
			if(isThreadEnabled(stateIndex, op->getThreadId(), threadState)){
				resultSet.insert(op->getThreadId());
			}
		}
	}
	return resultSet;			// signifies no such thread found!
}*/

//in the returned pair, first is set of all threadIds, second is set of threadIds with executable tasks.
set<int> findHappenedBeforeEnabledThreads(int stateIndex, Operation *opJ, int threadIdOfOpI, bool isDPOR){
	Debug(cerr<<"fn:findHappenedBeforeEnabledThreads"<<endl);
	set<int> allThreadSet;
	int j = opJ->getOpIndex(); //prior to calling FindTarget opJ (even if not executed in current sequence) was temporarily given the latest index of the sequence.
	int i = stateIndex;
	Debug(cerr<<" j = "<<j<<" i = "<<i<<endl);
	for(int k = j-1; k > i; k--){
		Operation *op = getOperation(k);
		if( if_i_HB_task(op,opJ,isDPOR) ){
			Debug(cerr<<" k :: "<<k<<endl);
			Thread threadState = getThreadState(stateIndex, op->getThreadId());
			if(op->getThreadId() == threadIdOfOpI)
				continue;
			if(isThreadEnabled(stateIndex, op->getThreadId(), threadState)){
				allThreadSet.insert(op->getThreadId());
			}
		}
	}
	return allThreadSet;
}


/*
 * Need to compute all tasks (can't exit just after getting first thread for a task as you don't know when that thread can be populated again in below computation.
 */
map<int, set<int> > findHappenedBeforeEnqueuedTasks(int stateIndex, Operation *opJ, bool isDPOR){				//returns set of tasks grouped by threads
	Debug(cerr<<"Begin:: fn-findHappenedBeforeEnqueuedTasks, stateIndex : "<<stateIndex<<" jOpIndex : "<<opJ->getOpIndex()<<endl);
	map<int, set<int> > taskIds;
	int j = opJ->getOpIndex();
	int i = stateIndex;
	DebugSelective(cerr<<" j = "<<j<<" i = "<<i<<endl);
	for(int k = j-1; k > i; k--){
		Operation *op = getOperation(k);
		if( if_i_HB_task(op,opJ,isDPOR) ){
			DebugSelective(cerr<<" k :: "<<k<<endl);
			Thread threadState = getThreadState(stateIndex, op->getThreadId());
			if(isThreadEnabled(stateIndex, op->getThreadId(), threadState)){
				if(isThreadLooping(threadState)){
					DebugSelective(cerr<<" here..."<<endl);
					int executableTaskId = threadState.getExecutableTask();
					if(threadState.isTaskEnqueued(op->getTaskId()) && (executableTaskId != op->getTaskId()) ){
						DebugSelective(cerr<<" there..."<<endl);
						if(executableTaskId == -1){
							cerr<<"ERROR::executableTaskId is -1!"<<endl;
							throw error;
						}
						PostOperation * pop = dynamic_cast<PostOperation *>(getOperation(getParentPostIndex(executableTaskId)));
						int beginIndexOfExecutableTaskInState_s = pop->getBeginOpIndex();
						if(!if_i_HB_j(beginIndexOfExecutableTaskInState_s, k, pop->getThreadId())){
							/* there is no need to check for the above condition before identifying
							 * blocked tasks to be reordered with corresponding executable tasks.
							 * However, its not incorrect to do this check, just that if no blocked
							 * task gets identified due to this check then unnecessarily expensive
							 * BacktrackEager will be performed.
							 */
							map<int, set<int> >::iterator it = taskIds.find(op->getThreadId());
							set<int> tasks;
							if(it != taskIds.end()){
								tasks = it->second;
							}
							tasks.insert(op->getTaskId());
							taskIds[op->getThreadId()] = tasks;
							DebugSelective(cerr<<" where..."<<endl);
						}
					}
				}
			}
		}
	}
	DebugSelective(cerr<<" End:: fn-findHappenedBeforeEnqueuedTasks"<<endl);
	return taskIds;
}

}


#endif /* EMDPORVECTORCLOCK_H_ */
