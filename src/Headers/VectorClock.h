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

#ifndef VECTORCLOCK_H_
#define VECTORCLOCK_H_

#include <iterator>
#include <utility>
#include <vector>
#include <map>
#include <set>

#include <Entities.h>
#include <BookKeeping.h>

using namespace std;
using namespace emdpor_bookKeeper;

namespace dpor_vc{

int **threadClock, **indexClock;
int MAX_THREADS=0, MAX_INDICES=0;
int *backupThreadClockVector, *backupIndexClockVector;

void resetClock(){
	for(int i=0;i<MAX_THREADS;i++)
		for(int j=0;j<MAX_THREADS;j++)
			threadClock[i][j]=0;

	for(int i=0;i<MAX_INDICES;i++)
		for(int j=0;j<MAX_THREADS;j++)
			indexClock[i][j]=0;
}

void initClock(int programLength, int noOfThreads){
	// ideal value is maximum thread Id + 1 for array(vector clock) indexing without error.
	noOfThreads++;
	threadClock = new int*[noOfThreads];
	indexClock = new int*[programLength];
	backupIndexClockVector = new int[noOfThreads];
	backupThreadClockVector = new int[noOfThreads];
	for(int i = 0; i < noOfThreads; i++)
		threadClock[i] = new int[noOfThreads];
	for(int i = 0; i < programLength; i++)
		indexClock[i] = new int[noOfThreads];
	MAX_THREADS=noOfThreads, MAX_INDICES=programLength;
	resetClock();
}

void deleteClock(){
	for(int i = 0; i < MAX_THREADS; i++)
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
	for(int i=0; i<MAX_THREADS; ++i)
		cerr<<setw(4)<<i;
	cerr<<endl;
	for(int i = 0; i < MAX_INDICES; ++i){
		cerr<<i<<"\t";
		for(int j = 0; j < MAX_THREADS; ++j){
			cerr<<setw(4)<<indexClock[i][j];
		}
		cerr<<endl;
	}
}

void dumpThreadClock(){
	cerr<<"Dumping Thread Clock..."<<endl;
	cerr<<"Thread\t";
	for(int i=0; i<MAX_THREADS; ++i)
		cerr<<setw(4)<<i;
	cerr<<endl;
	for(int i = 0; i < MAX_THREADS; ++i){
		cerr<<i<<"\t";
		for(int j = 0; j < MAX_THREADS; ++j){
			cerr<<setw(4)<<threadClock[i][j];
		}
		cerr<<endl;
	}
}

void dumpClock(){
	dumpThreadClock();
	dumpIndexClock();
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
//	Debug(cerr<<"MAX_THREADS : "<<MAX_THREADS<<endl);
	for(int i =0; i<MAX_THREADS; i++){
//		Debug(cerr<<"i : "<<i<<endl);
		inOutVector1[i] = (inOutVector1[i] > inVector2[i]) ? inOutVector1[i] : inVector2[i];
	}
}

void addHBEdge(int sourceIndex, int destIndex, int destIndexThreadId){
	Debug(cerr<<"addHBEdge() called----sourceIndex : "<<sourceIndex<<". destIndex : "<<destIndex<<", destIndexThreadId : "<<destIndexThreadId<<endl);
	int clockVector[MAX_THREADS];
	initializeArray(clockVector, MAX_THREADS);
	getPointWiseMaxVector(clockVector, indexClock[sourceIndex]);
	clockVector[destIndexThreadId] = destIndex;
	getPointWiseMaxVector(threadClock[destIndexThreadId], clockVector);
	getPointWiseMaxVector(indexClock[destIndex], clockVector);
}

int temp_destIndex=-1, temp_destIndexThreadId=-1;

void backup(int destIndex, int destIndexThreadId){
	temp_destIndex = destIndex;
	temp_destIndexThreadId = destIndexThreadId;
	for(int i=0; i<MAX_THREADS; ++i){
		backupThreadClockVector[i] = threadClock[destIndexThreadId][i];
		backupIndexClockVector[i] = indexClock[destIndex][i];
	}
}

void addTempHBEdges(int sourceIndex, int destIndex, int destIndexThreadId){
	//	Debug(cerr<<"fn:addTempHBEdges called, input - sourceIndex : "<<sourceIndex<<" destIndex : "<< destIndex << endl);
	backup(destIndex, destIndexThreadId);
	addHBEdge(sourceIndex, destIndex, destIndexThreadId);
	//	Debug(cerr<<"exiting fn:addTempHBEdges, sourceNode : "<<sourceNode<<" destNode : "<< destNode << endl);
}

void restore(int destIndex, int destIndexThreadId){
	for(int i=0; i<MAX_THREADS; ++i){
		threadClock[destIndexThreadId][i] = backupThreadClockVector[i];
		indexClock[destIndex][i] = backupIndexClockVector[i];
	}
}

void removeTempHBEdge(){
	restore(temp_destIndex, temp_destIndexThreadId);
	temp_destIndex=-1;
	temp_destIndexThreadId=-1;
}

void addHeapReadHBEdges(pair<long,int> key, int writeAccess, int writeAccessThreadId){
	vector<int> readIndices = getHeapReadIndices(key);
	for(vector<int>::iterator it = readIndices.begin(); it != readIndices.end(); ++it)
		addHBEdge(*it, writeAccess, writeAccessThreadId);
}

void addHeapWriteHBEdges(pair<long,int> key, int access, int accessThreadId){
	vector<int> writeIndices = getHeapWriteIndices(key);
	for(vector<int>::iterator it = writeIndices.begin(); it != writeIndices.end(); ++it){
		addHBEdge(*it, access, accessThreadId);
	}
}

void addStackReadHBEdges(pair<string,int> key, int writeAccess, int writeAccessThreadId){
	vector<int> readIndices = getStackReadIndices(key);
	for(vector<int>::iterator it = readIndices.begin(); it != readIndices.end(); ++it)
		addHBEdge(*it, writeAccess, writeAccessThreadId);
}

void addStackWriteHBEdges(pair<string,int> key, int access, int accessThreadId){
	vector<int> writeIndices = getStackWriteIndices(key);
	for(vector<int>::iterator it = writeIndices.begin(); it != writeIndices.end(); ++it){
		addHBEdge(*it, access,accessThreadId);
	}
}

void addPostHBEdgesInDPOR(int postIndex, int destThreadId, int postIndexThreadId){
	vector< pair<int,int> > dependentPosts = getPostsToThread(destThreadId);
	for(int i=0; i<(int)dependentPosts.size(); ++i){
		pair<int,int> aPair = dependentPosts.at(i);
		int depPostIndex = aPair.first;
		addHBEdge(depPostIndex, postIndex, postIndexThreadId);
	}
}

void addUnlockToLockHBEdges(int objectId, int threadId, int lockIndex){
	map<long, vector<int> >::iterator it = unlockMap.find(objectId);

	if(it!=unlockMap.end()){
		vector<int> unlockVector = it->second;
		for(vector<int>::iterator vit = unlockVector.begin(); vit != unlockVector.end(); ++vit){
			int unlockIndex = *vit;
			Operation *op = getOperation(unlockIndex);
			if(op->getThreadId() != threadId)
				addHBEdge(unlockIndex, lockIndex, threadId);
		}
	}
}

bool if_i_HB_j(int firstIndex, int secondIndex, int firstThread){
	if(firstIndex <= indexClock[secondIndex][firstThread])
		return true;
	return false;
}

bool if_i_HB_p(int firstIndex, int processId, int firstThread){
	Debug(cerr<<"firstIndex :"<<firstIndex<<" threadClock["<<processId<<"]["<<firstThread<<"] = "<<threadClock[processId][firstThread]<<endl);
	if(firstIndex <= threadClock[processId][firstThread]){
		Debug(cerr<<"returning true"<<endl);
		return true;
	}
	Debug(cerr<<"returning false"<<endl);
	return false;
}

// functions required for getting last dependent operation.

int getLastDependentHeapAccessInstIndexWithWrite(pair<long,int> key, int writeAccessIndex, bool isDPOR, int threadId, int opProgramIndex){
	Debug(cerr<<"getLastDependentHeapAccessInstIndexWithWrite() :: begin"<<endl);
	//	dumpHBGraph();
	vector<int> readIndices = getHeapReadIndices(key);
	vector<int> writeIndices = getHeapWriteIndices(key);
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
				if( !if_i_HB_p(i,threadId,lastThreadId) ){
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
				if( !if_i_HB_p(i,threadId,lastThreadId) ){
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
				if( !if_i_HB_p(i,threadId,lastThreadId) ){
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
				if( !if_i_HB_p(i,threadId,lastThreadId) ){
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

int getLastDependentHeapAccessInstIndexWithRead(pair<long,int> key, int readAccessIndex, bool isDPOR, int threadId, int opProgramIndex){
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
				if( !if_i_HB_p(i,threadId,lastThreadId) ){
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


int getLastDependentStackAccessInstIndexWithWrite(pair<string,int> key, int writeAccessIndex, bool isDPOR, int threadId, int opProgramIndex){

	vector<int> readIndices = getStackReadIndices(key);
	vector<int> writeIndices = getStackWriteIndices(key);

	int lastRead=-1, lastWrite=-1;
	if(!readIndices.empty() && !writeIndices.empty()){
		Debug(cerr<<"case :: !readIndices.empty() && !writeIndices.empty()"<<endl);
		for(vector<int>::reverse_iterator it = readIndices.rbegin(); it != readIndices.rend(); ++it){
			int i = *it;
			int lastThreadId=-1;
			Operation *op = getOperation(i);
			if(op!=NULL){
				lastThreadId = op->getThreadId();
				if( !if_i_HB_p(i,threadId,lastThreadId) ){
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
				if( !if_i_HB_p(i,threadId,lastThreadId) ){
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
				if( !if_i_HB_p(i,threadId,lastThreadId) ){
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
				if( !if_i_HB_p(i,threadId,lastThreadId) ){
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
int getLastDependentStackAccessInstIndexWithRead(pair<string,int> key, int readAccessIndex, bool isDPOR, int threadId, int opProgramIndex){
	//dumpHBGraph();
	vector<int> writeIndices = getStackWriteIndices(key);

	if(!writeIndices.empty()){
		int lastWrite=-1;
		for(vector<int>::reverse_iterator it = writeIndices.rbegin(); it != writeIndices.rend(); ++it){
			int i = *it;
			int lastThreadId=-1;
			Operation *op = getOperation(i);
			if(op!=NULL){
				lastThreadId = op->getThreadId();
				if( !if_i_HB_p(i,threadId,lastThreadId) ){
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
int getLastDependentLockInstIndex(long objId, int lockIndex, int threadId, int opProgramIndex){
	vector<int> lockIndices = getLocksOnId(objId);

	if(!lockIndices.empty()){
		for(vector<int>::reverse_iterator it = lockIndices.rbegin(); it != lockIndices.rend(); ++it){
			int i = *it;
			int lastThreadId=-1;
			Operation *op = getOperation(i);
			if(op!=NULL){
				lastThreadId = op->getThreadId();
				if( !if_i_HB_p(i,threadId,lastThreadId) ){
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

// it gets called only in case of isDPOR is true, so no need of bool argument
int getLastPostToSameThreadInstIndex(int postIndex, int threadId, int destinationThreadId, int postProgramIndex){
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
				if( !if_i_HB_p(i,threadId,lastThreadId) ){
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

void addExplicitHBEdges(int target, int targetThreadId){
	set<int> sourceSet =  explicitDependenciesBackup.getSources(target);
	if(sourceSet.empty())
		return;
	int targetCL = mapProgramIndexToControlLocation(target);
	for(set<int>::iterator it = sourceSet.begin(); it != sourceSet.end(); ++it){
		int sourceCL = mapProgramIndexToControlLocation(*it);
		Debug(cerr<<"Adding explicit HBEdge"<<endl);
		addHBEdge(sourceCL, targetCL, targetThreadId);
	}
}

// Helper functions for findTarget

set<int> findTaskExecutableHappenedBeforeThread(int stateIndex, int jOpIndex, int jThreadId, int threadIdOfOpI){
	Debug(cerr<<" stateIndex : "<<stateIndex<<" jOpIndex : "<<jOpIndex<<endl);
	set<int> resultSet;
	int j = jOpIndex;
	int i = stateIndex;
	Debug(cerr<<" j = "<<j<<" i = "<<i<<endl);
	for(int k = j-1; k > i; k--){
		Operation *op = getOperation(k);
		if( if_i_HB_p(k,jThreadId,op->getThreadId()) ){
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

set<int> findHappenedBeforeEnabledThreads(int stateIndex, int jOpIndex, int threadIdOfOpI){
	Debug(cerr<<" stateIndex : "<<stateIndex<<" jOpIndex : "<<jOpIndex<<endl);
	set<int> resultSet;
	int j = jOpIndex;
	int i = stateIndex;
	Debug(cerr<<" j = "<<j<<" i = "<<i<<endl);
	for(int k = j-1; k > i; k--){
		Operation *op = getOperation(k);
		if( if_i_HB_j(k,j,op->getThreadId()) ){
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
}


/*
 * Need to compute all tasks (can't exit just after getting first thread for a task as you don't know when that thread can be populated again in below computation.
 */
map<int, set<int> > findHappenedBeforeEnqueuedTasks(int stateIndex, int jOpIndex){				//returns set of tasks grouped by threads
	Debug(cerr<<" stateIndex : "<<stateIndex<<" jOpIndex : "<<jOpIndex<<endl);
	map<int, set<int> > taskIds;
	int j = jOpIndex;
	int i = stateIndex;
	Debug(cerr<<" j = "<<j<<" i = "<<i<<endl);
	for(int k = j-1; k > i; k--){
		Operation *op = getOperation(k);
		if( if_i_HB_j(k,j,op->getThreadId()) ){
			Debug(cerr<<" k :: "<<k<<endl);
			Thread threadState = getThreadState(stateIndex, op->getThreadId());
			//			if(isThreadEnabled(stateIndex, op->getThreadId(), threadState)){
			if(isThreadLooping(threadState)){
				Debug(cerr<<" here..."<<endl);
				if(threadState.isTaskEnqueued(op->getTaskId())){
					Debug(cerr<<" there..."<<endl);
					map<int, set<int> >::iterator it = taskIds.find(op->getThreadId());
					set<int> tasks;
					if(it != taskIds.end()){

						tasks = it->second;
					}
					tasks.insert(op->getTaskId());
					taskIds[op->getThreadId()] = tasks;
					Debug(cerr<<" where..."<<endl);
				}
			}

		}
	}
	return taskIds;
}

}


#endif /* VECTORCLOCK_H_ */
