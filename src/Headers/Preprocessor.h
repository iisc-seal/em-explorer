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

#ifndef PREPROCESSOR_H_
#define PREPROCESSOR_H_

#include <string>
#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <Debug.h>
#include <Entities.h>
#include <ExplicitDependencies.h>

using namespace std;
using namespace emdpor_Entities;

enum ConfigType {
  DPOR_MODE = 2,
  EMDPOR_MODE = 3
};

namespace emdpor_preprocessor_private{



map<int, vector<int> > threadCode;								// key - threadId, values - vector of operation indices executed on that thread.
map<int, vector<int> > taskCode;								// key - taskId, values - vector of operation indices executed in that event handler.
/* key - operation index, Value - pointer to Operation Object.
 * contains the indices in the input trace as keys. This data structure
 * once populated does not get modified.
 */
map<int, Operation *>  programCode;
/* field sequence always holds the sequence explored upto the current state.
 * It is a vector containing indices of operations as given in the input trace.
 * We call these program indices.
 */
vector<int> sequence;
map<int, int> programIndexToControlLocationMap;					// key - program index, Value- Control location
map<long,int> preprocessingLockMap;
map<long,int> preprocessingBadUnLockMap;
int programLength = 0;
int configId=EMDPOR_MODE; 	//hardcoding since this repo branch is dedicated for emdpor algorithm

/* explicitDependenciesBackup - contains info about HB info  explicitly provided by dependence.txt.
 * explicitDependencies - An operation appearing as dest in dependence.txt can be enabled only when
 * all the ops appearing as its src in dependence.txt are executed. The field explicitDependencies
 * tracks info about src ops which are executed in current sequence, thus aiding in identification
 * of dest ops which can be enabled in the state reached by the current sequence.
 */
ExplicitDependencies explicitDependencies, explicitDependenciesBackup;


// Private Declarations & Definitions
Thread useLess;
Thread & nullThread = useLess;
map<int, Thread> *threadStateMap = new map<int, Thread>();




// Auxiliary Functions for "sequence"
int getLastControlLocation(){
	if(sequence.empty())
		return -1;
	else
		return sequence.size()-1;
}

/* programIndex    - Index of the operation as in the input trace (since we see input trace as a program we call this as program index)
 * controlLocation - index indicating the state at which the operation is executed in the current sequence
 */
inline void addIntoSequence(int programIndex, int controlLocation){
	sequence.push_back(programIndex);
	programIndexToControlLocationMap[programIndex] = controlLocation;
}



int mapContolLocationToProgramIndex(int cl){
	return sequence.at(cl);
}
/*
int mapProgramIndexToControlLocation(int PI){
	for( int i=0; i < sequence.size(); ++i){
		if( sequence.at(i) == PI )
			return i;
	}
	Debug(cerr<<"ERROR :: Couldn't map program index to control location in fn:mapProgramIndexToControlLocation for pi : "<< PI<<endl);
	return -1;		// couldn't map
}
 */

int mapProgramIndexToControlLocation(int PI){
	map<int, int> :: iterator it = programIndexToControlLocationMap.find(PI);
	if(it != programIndexToControlLocationMap.end())
		return it->second;
	cerr<<"ERROR :: Couldn't map program index to control location in fn:mapProgramIndexToControlLocation for pi : "<< PI<<endl;
	throw error;
	return -1;		// couldn't map
}

void pruneProgramIndexToControlLocationMap(){
	programIndexToControlLocationMap.erase(programIndexToControlLocationMap.begin(), programIndexToControlLocationMap.end());
	for(unsigned i=0; i<sequence.size(); ++i)
		programIndexToControlLocationMap[sequence.at(i)] = i;
}

void pruneProgramIndexToControlLocationMap(int index){
	programIndexToControlLocationMap.erase(programIndexToControlLocationMap.begin(), programIndexToControlLocationMap.end());
	for(int i=0; i<index; ++i)
		programIndexToControlLocationMap[sequence.at(i)] = i;
}


//	Thread state maintenance
inline void addThreadState(int threadId){
	map<int, Thread>::iterator it = threadStateMap->find(threadId);
	if(it == threadStateMap->end()){
		Thread t;
		t.setThreadId(threadId);
		threadStateMap->insert( pair<int, Thread>(threadId, t) );
	}
}

inline Thread & getThreadState(int threadId){
	map<int, Thread>::iterator it = threadStateMap->find(threadId);
	if(it == threadStateMap->end())
		return nullThread;
	Thread &t = it->second;
	return t;
}

inline void removeThreadState(int threadId){
	threadStateMap->erase(threadId);
}

//void deleteThreadStateMap(){
//	delete threadStateMap;
//}


//	Save Thread & Task Indices
void addThreadCode(int threadId, int index){
	map<int, vector<int> >::iterator it = threadCode.find(threadId);
	vector<int> threadIndices;
	if(it != threadCode.end()){
		threadIndices = it->second;
	}
	threadIndices.push_back(index);
	threadCode[threadId] = threadIndices;
}

void addTaskCode(int taskId, int index){
	map<int, vector<int> >::iterator it = taskCode.find(taskId);
	vector<int> taskIndices;
	if(it != taskCode.end()){
		taskIndices = it->second;
	}
	taskIndices.push_back(index);
	taskCode[taskId] = taskIndices;
}

void addCode(int threadId, int index){
	Thread &t = getThreadState(threadId);
	if(t.isNullThread())
		return;
	if(t.isLooping())
		addTaskCode(t.getTaskId(), index);

	else
		addThreadCode(threadId, index);

}

/* returns next instruction index of a thread after input Index (afterIndex).
 * afterIndex should be the index of an operation belonging to the same threadId as passed.
 */
int getNextThreadInstruction(int threadId, int afterIndex){
	//	Debug(cerr<<"fn:getNextThreadInstruction() - Beginning"<<endl);
	map<int, vector<int> >::iterator it = threadCode.find(threadId);
	if(it == threadCode.end())
		return -1;
	vector<int> threadIndices = it->second;
	if(afterIndex == -1)
		return *(threadIndices.begin());
	afterIndex = mapContolLocationToProgramIndex(afterIndex);
	vector<int>::iterator vit = find(threadIndices.begin(), threadIndices.end(), afterIndex);
	if(vit != threadIndices.end()){
		++vit;
		if(vit != threadIndices.end())
			return *vit;
	}

	return -1;
}

void dumpthreadIndicesMap(){
	cerr<<"Dumping Thread indices...!"<<endl;
	for(map<int, vector<int> >::iterator it = threadCode.begin(); it!=threadCode.end(); ++it){
		cerr<<"Thread : "<<it->first<<endl;
		vector<int> indices = it->second;
		for(vector<int>::iterator vit = indices.begin(); vit!=indices.end(); ++vit)
			cerr<<" "<<setw(3)<<*vit;
		cerr<<endl;
	}
	cerr<<endl;
}

int getMaxThreadId(){
	if(threadCode.empty())
		return 0;
	int maxThreadId = threadCode.rbegin()->first;
	Debug(cerr<<"Max Thread Id = "<<maxThreadId<<endl);
	return maxThreadId;
}

int getMaxTaskId(){
	if(taskCode.empty())
		return 0;
	int maxTaskId = taskCode.rbegin()->first;
	Debug(cerr<<"Max Task Id = "<<maxTaskId<<endl);
	return maxTaskId;
}

int getThreadCount(){
	return threadCode.size();
}

int getTaskCount(){
	return taskCode.size();
}

void dumptaskIndicesMap(){
	cerr<<"Dumping Task indices...!"<<endl;
	for(map<int, vector<int> >::iterator it = taskCode.begin(); it!=taskCode.end(); ++it){
		cerr<<"Task : "<<it->first<<endl;
		vector<int> indices = it->second;
		for(vector<int>::iterator vit = indices.begin(); vit!=indices.end(); ++vit)
			cerr<<" "<<setw(3)<<*vit;
		cerr<<endl;
	}
	cerr<<endl;
}

/*
 * Input :: afterIndex
 * pass -1, if you want begin operation which is the 1st operation of the task, for which no afterIndex is possible
 *
 */
int getNextTaskInstruction(int taskId, int afterIndex){		//returns next instruction index of a thread in some state(afterIndex)
	//	Debug(cerr<<"fn:getNextTaskInstruction() - Beginning"<<endl);
	//	Debug(cerr<<"afterIndex = "<<afterIndex<<endl);
	map<int, vector<int> >::iterator it = taskCode.find(taskId);
	if(it == taskCode.end())
		return -1;
	vector<int> taskIndices = it->second;
	if(afterIndex == -1)
		return *(taskIndices.begin());
	afterIndex = mapContolLocationToProgramIndex(afterIndex);
	vector<int>::iterator vit = find(taskIndices.begin(), taskIndices.end(), afterIndex);
	if(vit != taskIndices.end()){
		++vit;
		if(vit != taskIndices.end())
			return *vit;
	}
	return -1;
}


//	Trace auxiliary functions
void addOperationInTrace(int index, Operation *op){
	programCode[index]=op;
	programLength++;
}

//Operation *getOperationFromTrace(int index){
//	if(programCode.find(index) == programCode.end())
//		return NULL;
//	Operation *op = programCode.find(index)->second;
//	return op;
//}

void dumpTrace(){
	cerr<<"Dumping Whole Trace...!"<<endl;
	map<int, Operation*>::iterator it = programCode.begin();
	for( ; it != programCode.end(); ++it){
		it->second->dumpOpInfo();
		cerr<<endl<<endl<<endl;
	}
}

Operation * getOperationClone(int index){
	if(programCode.find(index) == programCode.end())
		return NULL;
	Operation *op = programCode.find(index)->second;
	Operation *opClone = op->getClone();
	return opClone;
}

// String Processing
string getFirstToken(string str, string delimiter){  //returns string upto delimiter
	//	Debug(cerr<<"Inside getFirstToken(), input string is : "<<str<<endl);
	string token;
	int index = str.find(delimiter);
	if(index!=-1)
		token = str.substr(0,index);
	else
		return str;
	return token;
}

string getLastToken(string str, string delimiter){  //returns string upto delimiter
	string token;
	int index = str.find_last_of(delimiter);
	if(index!=-1)
		token = str.substr(index+1);
	else
		return "";
	return token;
}

string getLeftOverString(string str, string delimiter){
	string leftOver;
	int index = str.find(delimiter);
	if(index==-1)
		return "";
	leftOver = str.substr(index+1);
	return leftOver;
}

//we use key passed only for debug purpose. We read the config file in a crude manner line by line.
string getKeyValue(string key, string str){
    return str.substr(str.find(":")+1);
}

bool isNumber(char c){
	if((c>='0' && c<='9') || c=='+' || c=='-')
		return true;
	return false;
}

int getInt(string str){
	const char *c = str.c_str();
	return atoi(c);
}

int getLong(string str){
	const char *c = str.c_str();
	return strtol(c,NULL,0);
}


/// Auxiliary Functions for "controlLocToProgIndexMap"
/*
int mapContolLocationToProgramIndex(int cl){
	map<int,int>::iterator it = controlLocToProgIndexMap.find(cl);
	if(it != controlLocToProgIndexMap.end())
		return it->second;
	return -1;
}
 */

}



namespace emdpor_preprocessor{

using namespace emdpor_preprocessor_private;

string traceFileName, dependenceFileName, exploredTraceFile, exploredTraceFolder, reportFilePath;
bool fileOrFolderOption=true;		//true for file
int frequencyInMins=5;

void deleteThreadStateMap(){
	delete threadStateMap;
}

// preprocessing
void preprocessOperation(Operation *op){
	if(op == NULL)
		return;

	Thread &t = getThreadState(op->getThreadId());
	LockOperation *lop;

	switch(op->getOpType()){
	case NATIVE_ENTRY:
	case THREAD_INIT:
		addThreadState(op->getThreadId());
		addThreadCode(op->getThreadId(), op->getOpIndex());
		addOperationInTrace(op->getOpIndex(), op);
		break;
	case NATIVE_EXIT:
	case THREAD_EXIT:
		addThreadCode(op->getThreadId(), op->getOpIndex());
		removeThreadState(op->getThreadId());
		addOperationInTrace(op->getOpIndex(), op);
		break;
	case LOOP:
		if(!t.isNullThread()){
			t.setLooper(true);
			addThreadCode(op->getThreadId(), op->getOpIndex());
			addOperationInTrace(op->getOpIndex(), op);
		}
		break;
	case BEGIN:
		if(!t.isNullThread()){
			t.setTaskId(op->getTaskId());
			addTaskCode(op->getTaskId(), op->getOpIndex());
			addOperationInTrace(op->getOpIndex(), op);
		}
		break;
	case END:
		if(!t.isNullThread()){
			addTaskCode(op->getTaskId(), op->getOpIndex());
			t.setTaskId(-1);
			addOperationInTrace(op->getOpIndex(), op);
		}
		break;
	case LOCK:
		op->setTaskId(t.getTaskId());
		addCode(op->getThreadId(), op->getOpIndex());
		addOperationInTrace(op->getOpIndex(), op);
		if(!t.isNullThread()){
			lop = dynamic_cast<LockOperation *>(op);
			preprocessingLockMap[lop->getObjectId()] = lop->getOpIndex();
		}
		break;
	case UNLOCK:
		op->setTaskId(t.getTaskId());
		addCode(op->getThreadId(), op->getOpIndex());
		addOperationInTrace(op->getOpIndex(), op);
		if(!t.isNullThread()){
			lop = dynamic_cast<LockOperation *>(op);
			if( preprocessingLockMap.find(lop->getObjectId()) == preprocessingLockMap.end()){
				preprocessingBadUnLockMap[lop->getObjectId()] = lop->getOpIndex();
			}
			else
				preprocessingLockMap.erase(lop->getObjectId());

		}
		break;
	default:
		op->setTaskId(t.getTaskId());
		addCode(op->getThreadId(), op->getOpIndex());
		addOperationInTrace(op->getOpIndex(), op);
		break;
	}
}



// reads configuration information from config.cfg file
void getConfig(const char *pathFile){
	string line;
	ifstream pathFileStream (pathFile);

	if (pathFileStream.is_open()){
		if( getline (pathFileStream,line)){
			traceFileName = getKeyValue("trace", line);
		}
		if( getline (pathFileStream,line)){
			dependenceFileName = getKeyValue("dependence", line);
		}
		if( getline (pathFileStream,line)){
			exploredTraceFile = getKeyValue("exploredTracesFile", line);
		}
		if( getline (pathFileStream,line)){
			exploredTraceFolder = getKeyValue("exploredTracesFolder", line);
		}
		if( getline (pathFileStream,line)){
			int option = getInt(getKeyValue("fileOrFolder", line));
			if(option==1)
				fileOrFolderOption=true;
			else
				fileOrFolderOption=false;
		}
		if( getline (pathFileStream,line)){
			configId = getInt(getKeyValue("whichConfig", line));
			configId = EMDPOR_MODE; //hardcoding as this repo branch is dedicated for emdpor
		}
		if( getline (pathFileStream,line)){
			reportFilePath = getKeyValue("report", line);
			cleanFile(reportFilePath);
		}
		if( getline (pathFileStream,line)){
			frequencyInMins = getInt(getKeyValue("reportingFrequencyInMinutes", line));
			frequencyInMins = 5;
			ofstream reportfile;
			reportfile.open (reportFilePath.c_str(), ios::app);
			reportfile<<"Reporting frequency : "<<frequencyInMins<<" minutes."<<endl;
			switch(configId){
			case DPOR_MODE: reportfile<<"Configuration : DPOR"<<endl; break;
			case EMDPOR_MODE: reportfile<<"Configuration : EM-DPOR"<<endl; break;
			default:reportfile<<"Configuration : EM-DPOR"<<endl;
			}
			reportfile.close();
		}
		if(fileOrFolderOption){
			Report(cleanFile(exploredTraceFile));
			Debug(cerr<<"fileORfolderOption = 1"<<endl);
		}
		else
			Debug(cerr<<"fileORfolderOption = 2"<<endl);
		Debug(cerr<<traceFileName<<endl;
		cerr<<dependenceFileName<<endl;
		cerr<<exploredTraceFile<<endl<<exploredTraceFolder<<endl;
		cerr<<configId<<endl);

	}

	pathFileStream.close();
}


// returns a pointer to newly created Operation object for each line of log processed, returns null for don't care Ops.
Operation * processLine(string str){
	if((str.length()<=2) || !isNumber(str[0])){
		return NULL;
	}

	Operation *retOp = NULL;
	string indexString = getFirstToken(str, " ");
	str = getLeftOverString(str, " ");
	int index = getInt(indexString);
	string operationName =  getFirstToken(str, " ");
	str = getLeftOverString(str, " ");
	//	if(operationName.compare("START")){
	//		//Operation result = new Operation();
	//	}
	if(!operationName.compare("THREADINIT")){
		string threadId = getKeyValue("tid", getFirstToken(str, " "));
		int tid = getInt(threadId);
		retOp = new Operation(index, THREAD_INIT, tid);
	}
	else if(!operationName.compare("NOP")){
		string threadId = getKeyValue("tid", getFirstToken(str, " "));
		int tid = getInt(threadId);
		retOp = new Operation(index, NOP, tid);
	}
	else if(!operationName.compare("ENABLE-LIFECYCLE")){
		string threadId = getKeyValue("tid", getFirstToken(str, " "));
		int tid = getInt(threadId);
		str = getLeftOverString(str, " ");
		string component = getKeyValue("component", getFirstToken(str, " "));
		str = getLeftOverString(str, " ");
		string idString =  getKeyValue("id", getFirstToken(str, " "));
		int id = getInt(idString);
		str = getLeftOverString(str, " ");
		string state = getKeyValue("state", getFirstToken(str, " "));
		retOp = new LifecycleOperation(index, ENABLE_LIFECYCLE, tid, component, id, state);
	}
	else if(!operationName.compare("TRIGGER-LIFECYCLE")){
		string threadId = getKeyValue("tid", getFirstToken(str, " "));
		int tid = getInt(threadId);
		str = getLeftOverString(str, " ");
		string component = getKeyValue("component", getFirstToken(str, " "));
		str = getLeftOverString(str, " ");
		string idString =  getKeyValue("id", getFirstToken(str, " "));
		int id = getInt(idString);
		str = getLeftOverString(str, " ");
		string state = getKeyValue("state", getFirstToken(str, " "));
		retOp = new LifecycleOperation(index, TRIGGER_LIFECYCLE, tid, component, id, state);
	}
	else if(!operationName.compare("TRIGGER-SERVICE")){
		string threadId = getKeyValue("tid", getFirstToken(str, " "));
		int tid = getInt(threadId);
		str = getLeftOverString(str, " ");
		string component = getKeyValue("component", getFirstToken(str, " "));
		str = getLeftOverString(str, " ");
		string idString =  getKeyValue("id", getFirstToken(str, " "));
		int id = getInt(idString);
		str = getLeftOverString(str, " ");
		string state = getKeyValue("state", getFirstToken(str, " "));
		retOp = new LifecycleOperation(index, TRIGGER_SERVICE, tid, component, id, state);
	}
	else if(!operationName.compare("ATTACH-Q")){
		string threadId = getKeyValue("tid", getFirstToken(str, " "));
		int tid = getInt(threadId);
		str = getLastToken(str, " ");
		string queue = getKeyValue("queue", str);
		long queueId = getLong(queue);
		retOp = new Attach_QOperation(index, ATTACH_Q, tid, queueId);
	}
	else if(!operationName.compare("LOOP")){
		string threadId = getKeyValue("tid", getFirstToken(str, " "));
		int tid = getInt(threadId);
		str = getLastToken(str, " ");
		string queue = getKeyValue("queue", str);
		long queueId = getLong(queue);
		retOp = new Attach_QOperation(index, LOOP, tid, queueId);
	}
	else if(!operationName.compare("POST")){
		string threadId = getKeyValue("src", getFirstToken(str, " "));
		int tid = getInt(threadId);
		str = getLeftOverString(str, " ");
		string postedTaskId = getKeyValue("msg", getFirstToken(str, " "));
		int tskid = getInt(postedTaskId);
		str = getLeftOverString(str, " ");
		string destThreadId = getKeyValue("dest", getFirstToken(str, " "));
		int destTid = getInt(destThreadId);
		retOp = new PostOperation(index, POST, tid, tskid, destTid);
	}
	else if(!operationName.compare("NATIVE-POST")){
		string threadId = getKeyValue("src", getFirstToken(str, " "));
		int tid = getInt(threadId);
		str = getLeftOverString(str, " ");
		string postedTaskId = getKeyValue("msg", getFirstToken(str, " "));
		int tskid = getInt(postedTaskId);
		str = getLeftOverString(str, " ");
		string destThreadId = getKeyValue("dest", getFirstToken(str, " "));
		int destTid = getInt(destThreadId);
		retOp = new PostOperation(index, NATIVE_POST, tid, tskid, destTid);
	}
	else if(!operationName.compare("UI-POST")){
		string threadId = getKeyValue("src", getFirstToken(str, " "));
		int tid = getInt(threadId);
		str = getLeftOverString(str, " ");
		string postedTaskId = getKeyValue("msg", getFirstToken(str, " "));
		int tskid = getInt(postedTaskId);
		str = getLeftOverString(str, " ");
		string destThreadId = getKeyValue("dest", getFirstToken(str, " "));
		int destTid = getInt(destThreadId);
		retOp = new PostOperation(index, UI_POST, tid, tskid, destTid);
	}
	else if(!operationName.compare("DELAY-POST")){
		string threadId = getKeyValue("src", getFirstToken(str, " "));
		int tid = getInt(threadId);
		str = getLeftOverString(str, " ");
		string postedTaskId = getKeyValue("msg", getFirstToken(str, " "));
		int tskid = getInt(postedTaskId);
		str = getLeftOverString(str, " ");
		string destThreadId = getKeyValue("dest", getFirstToken(str, " "));
		int destTid = getInt(destThreadId);
		retOp = new PostOperation(index, DELAY_POST, tid, tskid, destTid);
	}
	else if(!operationName.compare("CALL")){			//CALL = begin_task
		string threadId = getKeyValue("tid", getFirstToken(str, " "));
		int tid = getInt(threadId);
		str = getLastToken(str, " ");
		string msg = getKeyValue("msg", str);
		int taskId = getInt(msg);
		retOp = new Operation(index, BEGIN, tid, taskId);
	}
	else if(!operationName.compare("RET")){			//RET = end_task
		string threadId = getKeyValue("tid", getFirstToken(str, " "));
		int tid = getInt(threadId);
		str = getLastToken(str, " ");
		string msg = getKeyValue("msg", str);
		int taskId = getInt(msg);
		retOp = new Operation(index, END, tid, taskId);
	}
	else if(!operationName.compare("FORK")){
		string parentThreadId = getKeyValue("par-tid", getFirstToken(str, " "));
		int parentTid = getInt(parentThreadId);
		string childThreadId = getKeyValue("child-tid", getLastToken(str, " "));
		int childTid = getInt(childThreadId);
		retOp = new ForkOperation(index, FORK, parentTid, childTid);
	}

	else if(!operationName.compare("THREADEXIT")){
		string threadId = getKeyValue("tid", getFirstToken(str, " "));
		int tid = getInt(threadId);
		retOp = new Operation(index, THREAD_EXIT, tid);
	}
	else if(!operationName.compare("READ")){
		string threadId = getKeyValue("tid", getFirstToken(str, " "));
		int tid = getInt(threadId);
		str = getLeftOverString(str, " ");
		string objectId = getKeyValue("obj", getFirstToken(str, " "));
		long objId = getLong(objectId);
		str = getLeftOverString(str, " ");
		string className = getKeyValue("class", getFirstToken(str, ";"));
		string fieldString = getKeyValue("field", getLastToken(str, " "));
		int field = getInt(fieldString);
		retOp = new MemoryAccessOperation(index, READ, tid, objId, className, field);
	}
	else if(!operationName.compare("WRITE")){
		string threadId = getKeyValue("tid", getFirstToken(str, " "));
		int tid = getInt(threadId);
		str = getLeftOverString(str, " ");
		string objectId = getKeyValue("obj", getFirstToken(str, " "));
		long objId = getLong(objectId);
		str = getLeftOverString(str, " ");
		string className = getKeyValue("class", getFirstToken(str, ";"));
		string fieldString = getKeyValue("field", getLastToken(str, " "));
		int field = getInt(fieldString);
		retOp = new MemoryAccessOperation(index, WRITE, tid, objId, className, field);
	}
	else if(!operationName.compare("READ-STATIC")){
		string threadId = getKeyValue("tid", getFirstToken(str, " "));
		int tid = getInt(threadId);
		str = getLeftOverString(str, " ");
		string className = getKeyValue("class", getFirstToken(str, ";"));
		str = getLeftOverString(str, " ");
		string fieldString = getKeyValue("field", getFirstToken(str, " "));
		int field = getInt(fieldString);
		retOp = new MemoryAccessOperation(index, READ_STATIC, tid, -1, className, field);
	}
	else if(!operationName.compare("WRITE-STATIC")){
		string threadId = getKeyValue("tid", getFirstToken(str, " "));
		int tid = getInt(threadId);
		str = getLeftOverString(str, " ");
		string className = getKeyValue("class", getFirstToken(str, ";"));
		str = getLeftOverString(str, " ");
		string fieldString = getKeyValue("field", getFirstToken(str, " "));
		int field = getInt(fieldString);
		retOp = new MemoryAccessOperation(index, WRITE_STATIC, tid, -1, className, field);
	}
	else if(!operationName.compare("LOCK")){
		string threadID = getKeyValue("tid", getFirstToken(str, " "));
		int tid = getInt(threadID);
		string objString = getKeyValue("lock-obj", getLastToken(str, " "));
		long objId = getLong(objString);
		retOp = new LockOperation(index, LOCK, tid, objId);
	}

	else if(!operationName.compare("UNLOCK")){
		string threadID = getKeyValue("tid", getFirstToken(str, " "));
		int tid = getInt(threadID);
		string objString = getKeyValue("lock-obj", getLastToken(str, " "));
		long objId = getLong(objString);
		retOp = new LockOperation(index, UNLOCK, tid, objId);
	}

	else if(!operationName.compare("WAIT")){
		string threadId = getKeyValue("tid", getFirstToken(str, " "));
		int tid = getInt(threadId);
		retOp = new Operation(index, WAIT, tid);
	}

	else if(!operationName.compare("NOTIFY")){
		string threadId = getKeyValue("tid", getFirstToken(str, " "));
		int tid = getInt(threadId);
		string notifyTargetThreadString = getKeyValue("notifiedTid", getLastToken(str, " "));
		int notifyTargetThread = getLong(notifyTargetThreadString);
		retOp = new NotifyOperation(index, NOTIFY, tid, notifyTargetThread);
	}

	else if(!operationName.compare("ACCESS")){
		string threadId = getKeyValue("tid", getFirstToken(str, " "));
		int tid = getInt(threadId);
		retOp = new Operation(index, ACCESS, tid);
	}

	else if(!operationName.compare("NATIVE-ENTRY")){
		string threadId = getKeyValue("tid", getFirstToken(str, " "));
		int tid = getInt(threadId);
		string binderThreadString = getLastToken(str, "#");
		int binderThreadId = getInt(binderThreadString);
		retOp = new NativeOperation(index, NATIVE_ENTRY, tid, binderThreadId);
	}
	else if(!operationName.compare("NATIVE-EXIT")){
		string threadId = getKeyValue("tid", getFirstToken(str, " "));
		int tid = getInt(threadId);
		string binderThreadString = getLastToken(str, "#");
		int binderThreadId = getInt(binderThreadString);
		retOp = new NativeOperation(index, NATIVE_EXIT, tid, binderThreadId);
	}
	else if(!operationName.compare("ENABLE-EVENT")){
		string threadId = getKeyValue("tid", getFirstToken(str, " "));
		int tid = getInt(threadId);
		str = getLeftOverString(str, " ");
		string viewId = getKeyValue("view", getFirstToken(str, " "));
		long view = getLong(viewId);
		str = getLeftOverString(str, " ");
		string eventId = getKeyValue("event", getFirstToken(str, " "));
		int event = getInt(eventId);
		retOp = new EnableEventOperation(index, ENABLE_EVENT, tid, view, event);
	}
	else if(!operationName.compare("TRIGGER-EVENT")){
		string threadId = getKeyValue("tid", getFirstToken(str, " "));
		int tid = getInt(threadId);
		str = getLeftOverString(str, " ");
		string viewId = getKeyValue("view", getFirstToken(str, " "));
		long view = getLong(viewId);
		str = getLeftOverString(str, " ");
		string eventId = getKeyValue("event", getFirstToken(str, " "));
		int event = getInt(eventId);
		retOp = new EnableEventOperation(index, TRIGGER_EVENT, tid, view, event);
	}
	else if(!operationName.compare("TRIGGER-WINDOW-FOCUS")){			
		string threadId = getKeyValue("tid", getFirstToken(str, " "));
		int tid = getInt(threadId);
		str = getLastToken(str, " ");
		string windowHashId = getKeyValue("windowHash", str);
		long windowHash = getInt(windowHashId);
		retOp = new WindowFocusOperation(index, TRIGGER_WINDOW_FOCUS, tid, windowHash);
	}
	else if(!operationName.compare("ENABLE-WINDOW-FOCUS")){
		string threadId = getKeyValue("tid", getFirstToken(str, " "));
		int tid = getInt(threadId);
		str = getLastToken(str, " ");
		string windowHashId = getKeyValue("windowHash", str);
		long windowHash = getInt(windowHashId);
		retOp = new WindowFocusOperation(index, ENABLE_WINDOW_FOCUS, tid, windowHash);
	}
	else if(!operationName.compare("TRIGGER-BROADCAST")){
		string threadId = getKeyValue("tid", getFirstToken(str, " "));
		int tid = getInt(threadId);
		str = getLeftOverString(str, " ");
		string action = getKeyValue("action", getFirstToken(str, " "));
		str = getLeftOverString(str, " ");
		string component = getKeyValue("component", getFirstToken(str, " "));
		long l_component = getLong(component);
		str = getLeftOverString(str, " ");
		string intent = getKeyValue("intent", getFirstToken(str, " "));
		int i_intent = getInt(intent);
		str = getLeftOverString(str, " ");
		string onRecLater = getKeyValue("onRecLater", getFirstToken(str, " "));
		int i_onRecLater = getInt(onRecLater);
		str = getLeftOverString(str, " ");
		string state = getKeyValue("state", getFirstToken(str, " "));
		retOp = new TriggerBroadcastOperation(index, TRIGGER_BROADCAST, tid, action, l_component, i_intent, i_onRecLater, state);
	}
	return retOp;
}

bool processDependence(string str){
	if(str.length() < 9)
		return false;
	int src = getInt(getKeyValue("src", getFirstToken(str, " ")));
	int dest = getInt(getKeyValue("dest", getLastToken(str, " ")));
	explicitDependencies.addDependency(src, dest);
	explicitDependenciesBackup.addDependency(src, dest);
	return true;
}

void resetExplicitDependencies(){
	explicitDependencies = explicitDependenciesBackup;
}

}
#endif /* PREPROCESSOR_H_ */

