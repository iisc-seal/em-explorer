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
 * Defines important structures such as Thread, Task and all types of Operations.
 */


#ifndef ENTITIES_H_
#define ENTITIES_H_

#include <iostream>
#include <string>
#include <queue>
#include <algorithm>
#include <set>
#include <sstream>

#include <Util.h>


using namespace std;

namespace emdpor_Entities{

struct Thread {
	int threadId;
	int parentThreadId;
	bool isLoopSet;
	long qId;
	int taskId;						//returns the Id of task being executed on this thread.
	queue<int> Q;
	int notifiyCount;

public:
	Thread(){
		this->threadId=-1;
		this->parentThreadId=-1;
		this->isLoopSet=false;
		this->qId=-1;				//consider qId as don't care if isQAttached is false;
		this->taskId=-1;
		this->notifiyCount=0;
	}

	void dumpThread(){
		cerr<<"Thread Id : "<<threadId<<endl;
		cerr<<"Parent Thread Id : "<<parentThreadId<<endl;
		if(qId==-1)
			cerr<<"Is Queue Attached : False"<<endl;
		else
			cerr<<"Is Queue Attached : True"<<endl;
		cerr<<"Is Looping : "<<isLoopSet<<endl;
		cerr<<"Queue Id : "<<qId<<endl;
		cerr<<"Task Id : "<<taskId<<endl;
		if( qId==-1 || Q.empty())
			cerr<<"Queue is empty"<<endl;
		else{
			queue<int> q = queue<int>(Q);
			cerr<<"Queue : ";
			while (!q.empty()){
				std::cerr << " " << q.front();
				q.pop();
			}
			std::cerr << endl;
		}
		cerr<<"Notify Count : "<<notifiyCount<<endl;
	}

	void dumpQueue(){
		if( qId==-1 || Q.empty())
			cerr<<"Queue is empty"<<endl;
		else{
			queue<int> q = queue<int>(Q);
			int size = q.size();
			cerr<<"Queue : ";
			while (size != 0){
				std::cerr << " " << q.front();
				q.pop(); size--;
			}
			std::cerr << endl;
		}
	}

	void setThreadId(int threadId){
		this->threadId = threadId;
	}

	bool isNullThread(){
		return (threadId == -1);
	}

	void attachQ(long qId){
		this->qId=qId;
	}

	bool isQAttached(){
		return qId!=-1;
	}

	int getThreadId() const {
		return threadId;
	}

	int getParentThreadId() const {
		return parentThreadId;
	}

	void setParentThreadId(int parentThreadId) {
		this->parentThreadId = parentThreadId;
	}

	long getQId() const {
		return qId;
	}

	int getTaskId() const {
		return taskId;
	}

	void setTaskId(int taskId) {
		this->taskId = taskId;
	}

	void insertIntoQ(int taskId){
		Q.push(taskId);
	}

	int deque(){
		if(!Q.empty()){
			int result = Q.front();
			Q.pop();
			return result;
		}
		return -1;
	}

	queue<int> getQ(){
		return Q;
	}

	bool isLooping() const {
		return isLoopSet;
	}

	void setLooper(bool isLoopSet) {
		this->isLoopSet = isLoopSet;
	}

	int getNotifyCount(){
		return this->notifiyCount;
	}

	void incrementNotifyCount(){
		notifiyCount = notifiyCount+1;
	}

	// call only if notifiyCount > 0
	void decrementNotifyCount(){
		if(this->notifiyCount <= 0){
			cerr<<"ERROR::notify count is already zero."<<endl; throw error;
		}
		notifiyCount = notifiyCount-1;
	}

	bool isTaskEnqueued(int taskId){
		queue<int> q = queue<int>(Q);
		int size = q.size();
		for(int i=0; i<size; i++){
			if(q.front() == taskId)
				return true;
			q.pop();
		}
		return false;
	}

	int getTaskNearestFoQ(set<int> tasks){
		queue<int> q = queue<int>(Q);
		int size = q.size();
		for(int i=0; i<size; i++){
			if(tasks.find(q.front()) != tasks.end())
				return q.front();
			q.pop();
		}
		return -1;
	}

	int getFrontOfQueue(){
		if(qId != -1 && !Q.empty())
			return Q.front();
		return -1;
	}

	int getExecutableTask(){
		if(qId != -1 && isLoopSet){
			if(taskId != -1)
				return taskId;
			return getFrontOfQueue();
		}
		return -1;
	}
};

struct Lifecycle {
	long id;				//match over these two
	string component;
	string state;			//match over these two
public:
	Lifecycle(long id, string component, string state){
		this->id=id;
		this->component=component;
		this->state=state;
	}

	Lifecycle(long id, string state){
		this->id=id;
		this->component="";
		this->state=state;
	}

	const string& getComponent() const {
		return component;
	}

	void setComponent(const string& component) {
		this->component = component;
	}

	long getId() const {
		return id;
	}

	void setId(long id) {
		this->id = id;
	}

	const string& getState() const {
		return state;
	}

	void setState(const string& state) {
		this->state = state;
	}
};


struct Task {
	int taskId;
	int threadId;
	int parentTaskId;
	int parentPostInstIndex;
	int beginIndex;
	int endIndex;
public:
	Task(int taskId, int parentPostInstIndex){
		this->taskId=taskId;
		this->threadId=-1;
		this->parentTaskId=-1;
		this->parentPostInstIndex=parentPostInstIndex;
		this->beginIndex=-1;
		this->endIndex=-1;
	}

	int getBeginIndex() const {
		return beginIndex;
	}

	void setBeginIndex(int beginIndex) {
		this->beginIndex = beginIndex;
	}

	int getEndIndex() const {
		return endIndex;
	}

	void setEndIndex(int endIndex) {
		this->endIndex = endIndex;
	}

	int getParentPostInstIndex() const {
		return parentPostInstIndex;
	}

	void setParentPostInstIndex(int parentPostInstIndex) {
		this->parentPostInstIndex = parentPostInstIndex;
	}

	int getParentTaskId() const {
		return parentTaskId;
	}

	void setParentTaskId(int parentTaskId) {
		this->parentTaskId = parentTaskId;
	}

	int getTaskId() const {
		return taskId;
	}

	void setTaskId(int taskId) {
		this->taskId = taskId;
	}

	int getThreadId() const {
		return threadId;
	}

	void setThreadId(int threadId) {
		this->threadId = threadId;
	}
};

enum operationIds {START=0, THREAD_INIT, THREAD_EXIT, ENABLE_LIFECYCLE, TRIGGER_LIFECYCLE, ATTACH_Q,
	NATIVE_ENTRY, NATIVE_EXIT, LOOP, POST, NATIVE_POST, UI_POST, DELAY_POST, BEGIN, END, READ, READ_STATIC, WRITE,
	WRITE_STATIC, ACCESS, ENABLE_EVENT, TRIGGER_EVENT, LOCK, FORK, JOIN, UNLOCK, WAIT, NOTIFY, INSTANCE_INTENT,
	ENABLE_WINDOW_FOCUS, TRIGGER_WINDOW_FOCUS, TRIGGER_BROADCAST, TRIGGER_SERVICE, NOP};

static const char * enumOpName[] = {"START", "THREADINIT", "THREADEXIT", "ENABLE-LIFECYCLE",
		"TRIGGER-LIFECYCLE", "ATTACH-Q", "NATIVE-ENTRY", "NATIVE-EXIT",
		"LOOP", "POST", "NATIVE-POST", "UI-POST", "DELAY-POST", "BEGIN", "END", "READ", "READ-STATIC", "WRITE",
		"WRITE-STATIC", "ACCESS", "ENABLE-EVENT", "TRIGGER-EVENT", "LOCK", "FORK", "JOIN", "UNLOCK", "WAIT", "NOTIFY", "INSTANCE-INTENT",
		"ENABLE-WINDOW-FOCUS", "TRIGGER-WINDOW-FOCUS", "TRIGGER-BROADCAST", "TRIGGER-SERVICE", "NOP"};

struct Operation{
protected:
	int opIndex;
	int opId;
	int threadId;
	int taskId;
public:
	Operation(int index, int opId, int threadId){
		this->opIndex=index;
		this->opId=opId;
		this->threadId=threadId;
		this->taskId=-1;
	}
	Operation(int index, int opId, int threadId, int taskId){
		this->opIndex=index;
		this->opId=opId;
		this->threadId=threadId;
		this->taskId=taskId;
	}

	virtual ~Operation(){}

	virtual void dumpOpInfo(){
		cerr<<"Operation Index : "<<opIndex<<endl;
		cerr<<"Operation ID : "<<opId<<endl;
		cerr<<"Operation Name : "<<enumOpName[opId]<<endl;
		cerr<<"Thread ID : "<<threadId<<endl;
		cerr<<"Task ID : "<<taskId<<endl;
	}

	virtual string writeToLogWithFormatting(){
		std::stringstream sstm;
		for(int i=1; i<threadId; i++)
				sstm << "\t";
		if(opId == THREAD_INIT || opId == THREAD_EXIT || opId == NOP)
			sstm << opIndex<<" "<<enumOpName[opId]<<" tid:"<<threadId;
		else
			sstm << opIndex<<" "<<enumOpName[opId]<<" tid:"<<threadId<<" msg:"<<taskId;
		
		return sstm.str();
	}

	virtual string writeToLog(){
		std::stringstream sstm;
		if(opId == THREAD_INIT || opId == THREAD_EXIT || opId == NOP)
			sstm << opIndex<<" "<<enumOpName[opId]<<" tid:"<<threadId;
		else
			sstm << opIndex<<" "<<enumOpName[opId]<<" tid:"<<threadId<<" msg:"<<taskId;
		
		return sstm.str();
	}

	virtual Operation * getClone(){
		Operation *opClone = new Operation(*this);
		return opClone;
	}

	int getOpId() const {
		return opId;
	}

	void setOpId(int opId) {
		this->opId = opId;
	}

	int getOpIndex() const {
		return opIndex;
	}

	void setOpIndex(int opIndex) {
		this->opIndex = opIndex;
	}

	int getTaskId() const {
		return taskId;
	}

	void setTaskId(int taskId) {
		this->taskId = taskId;
	}

	int getThreadId() const {
		return threadId;
	}

	void setThreadId(int threadId) {
		this->threadId = threadId;
	}
};

struct MemoryAccessOperation:public Operation{
protected:
	long objectId;
	string className;
	int field;
public:
	MemoryAccessOperation(int index, int opId, int threadId, long objectId, string className, int field):Operation(index, opId, threadId){
		this->objectId=objectId;
		this->className=className;
		this->field=field;
	}

	MemoryAccessOperation(int index, int opId, int threadId, int taskId, long objectId, string className, int field):Operation(index, opId, threadId, taskId){
		this->objectId=objectId;
		this->className=className;
		this->field=field;
	}


	virtual MemoryAccessOperation * getClone(){
		MemoryAccessOperation *opClone = new MemoryAccessOperation(*this);
		return opClone;
	}

	virtual void dumpOpInfo(){
		Operation::dumpOpInfo();
		if(objectId!=-1) cerr<<"Object Id : 0x"<<hex<<objectId<<dec<<endl;
		else cerr<<"Object Id : "<<objectId<<endl;
		cerr<<"Class Name : "<<className<<endl;
		cerr<<"Field : "<<field<<endl;
	}

	virtual string writeToLogWithFormatting(){
		std::stringstream sstm;
		for(int i=1; i<threadId; i++)
			sstm << "\t";
		sstm << opIndex<<" "<<enumOpName[opId]<<" tid:"<<threadId<<" obj:"<<objectId<<" class:"<<className<<"; field:"<<field;
		return sstm.str();
	}

	virtual string writeToLog(){
		std::stringstream sstm;
		
		sstm << opIndex<<" "<<enumOpName[opId]<<" tid:"<<threadId<<" obj:"<<objectId<<" class:"<<className<<"; field:"<<field;
		return sstm.str();
	}

	const string& getClassName() const {
		return className;
	}

	void setClassName(const string& className) {
		this->className = className;
	}

	int getField() const {
		return field;
	}

	void setField(int field) {
		this->field = field;
	}

	long getObjectId() const {
		return objectId;
	}

	void setObjectId(long objectId) {
		this->objectId = objectId;
	}
};

struct LockOperation:public Operation{
protected:
	long objectId;
public:
	LockOperation(int index, int opId, int threadId, long objectId):Operation(index, opId, threadId){
		this->objectId=objectId;
	}

	LockOperation(int index, int opId, int threadId, int taskId, long objectId):Operation(index, opId, threadId, taskId){
		this->objectId=objectId;
	}



	virtual LockOperation * getClone(){
		LockOperation *opClone = new LockOperation(*this);
		return opClone;
	}
	
	virtual void dumpOpInfo(){
		Operation::dumpOpInfo();
		cerr<<"Object Id : 0x"<<hex<<objectId<<dec<<endl;
	}

	virtual string writeToLogWithFormatting(){
		std::stringstream sstm;
		for(int i=1; i<threadId; i++)
			sstm << "\t";
		sstm << opIndex<<" "<<enumOpName[opId]<<" tid:"<<threadId<<" lock-obj:"<<objectId;
		return sstm.str();
	}

	virtual string writeToLog(){
		std::stringstream sstm;
		
		sstm << opIndex<<" "<<enumOpName[opId]<<" tid:"<<threadId<<" lock-obj:"<<objectId;
		return sstm.str();
	}

	long getObjectId() const {
		return objectId;
	}

	void setObjectId(long objectId) {
		this->objectId = objectId;
	}
};

struct NativeOperation:public Operation{
protected:
	int binderThreadId;
public:
	NativeOperation(int index, int opId, int threadId, int binderThreadId):Operation(index, opId, threadId){
		this->binderThreadId=binderThreadId;
	}

	NativeOperation(int index, int opId, int threadId, int taskId, int binderThreadId):Operation(index, opId, threadId, taskId){
		this->binderThreadId=binderThreadId;
	}


	virtual NativeOperation * getClone(){
		NativeOperation *opClone = new NativeOperation(*this);
		return opClone;
	}

	virtual void dumpOpInfo(){
		Operation::dumpOpInfo();
		cerr<<"Binder Thread Id : "<<binderThreadId<<endl;
	}

	virtual string writeToLog(){
		std::stringstream sstm;
		sstm << opIndex<<" "<<enumOpName[opId]<<" tid:"<<threadId<<" binder#"<<binderThreadId;
		return sstm.str();
	}


	int getBinderThreadId() const {
		return binderThreadId;
	}

	void setBinderThreadId(int binderThreadId) {
		this->binderThreadId = binderThreadId;
	}
};

struct NotifyOperation:public Operation{
protected:
	int notifyTagetThreadId;
public:
	NotifyOperation(int index, int opId, int threadId, int notifyTagetThreadId):Operation(index, opId, threadId){
		this->notifyTagetThreadId=notifyTagetThreadId;
	}

	NotifyOperation(int index, int opId, int threadId, int taskId, int notifyTagetThreadId):Operation(index, opId, threadId, taskId){
		this->notifyTagetThreadId=notifyTagetThreadId;
	}


	virtual NotifyOperation * getClone(){
		NotifyOperation *opClone = new NotifyOperation(*this);
		return opClone;
	}

	virtual void dumpOpInfo(){
		Operation::dumpOpInfo();
		cerr<<"Notify Target Thread Id : "<<notifyTagetThreadId<<endl;
	}

	virtual string writeToLogWithFormatting(){
		std::stringstream sstm;
		for(int i=1; i<threadId; i++)
			sstm << "\t";
		sstm << opIndex<<" "<<enumOpName[opId]<<" tid:"<<threadId<<" target:"<<notifyTagetThreadId;
		return sstm.str();
	}

	virtual string writeToLog(){
		std::stringstream sstm;
		
		sstm << opIndex<<" "<<enumOpName[opId]<<" tid:"<<threadId<<" target:"<<notifyTagetThreadId;
		return sstm.str();
	}

	int getNotifyTagetThreadId() const {
		return notifyTagetThreadId;
	}

	void setNotifyTagetThreadId(int notifyTagetThreadId) {
		this->notifyTagetThreadId = notifyTagetThreadId;
	}
};

struct ForkOperation:public Operation{
protected:
	int childThreadId;
public:
	ForkOperation(int index, int opId, int threadId, int childThreadId):Operation(index, opId, threadId){
		this->childThreadId=childThreadId;
	}
	ForkOperation(int index, int opId, int threadId, int taskId, int childThreadId):Operation(index, opId, threadId, taskId){
		this->childThreadId=childThreadId;
	}
	
	virtual ForkOperation * getClone(){
		ForkOperation *opClone = new ForkOperation(*this);
		return opClone;
	}
	virtual void dumpOpInfo(){
		Operation::dumpOpInfo();
		cerr<<"Forked Thread Id : "<<childThreadId<<endl;
	}

	virtual string writeToLogWithFormatting(){
		std::stringstream sstm;
		for(int i=1; i<threadId; i++)
			sstm << "\t";
		sstm << opIndex<<" "<<enumOpName[opId]<<" par-tid:"<<threadId<<" child-tid:"<<childThreadId;
		return sstm.str();
	}

	virtual string writeToLog(){
		std::stringstream sstm;
		
		sstm << opIndex<<" "<<enumOpName[opId]<<" par-tid:"<<threadId<<" child-tid:"<<childThreadId;
		return sstm.str();
	}

	int getChildThreadId() const {
		return childThreadId;
	}

	void setChildThreadId(int childThreadId) {
		this->childThreadId = childThreadId;
	}
};

struct LifecycleOperation:public Operation{
protected:
	string component;
	long id;
	string state;
public:
	LifecycleOperation(int index, int opId, int threadId, string comp, long id, string state):Operation(index, opId, threadId){
		this->component=comp;
		this->id=id;
		this->state=state;
	}
	LifecycleOperation(int index, int opId, int threadId, int taskId, string comp, long id, string state):Operation(index, opId, threadId, taskId){
		this->component=comp;
		this->id=id;
		this->state=state;
	}

	virtual LifecycleOperation * getClone(){
		LifecycleOperation *opClone = new LifecycleOperation(*this);
		return opClone;
	}
	void dumpOpInfo(){
		Operation::dumpOpInfo();
		cerr<<"Component : "<<component<<endl;
		cerr<<"ID : "<<id<<endl;
		cerr<<"state : "<<state<<endl;
	}

	virtual string writeToLogWithFormatting(){
		std::stringstream sstm;
		for(int i=1; i<threadId; i++)
			sstm << "\t";
		sstm << opIndex<<" "<<enumOpName[opId]<<" tid:"<<threadId<<" component:"<<component<<" id:"<<id<<" state:"<<state;
		return sstm.str();
	}

	virtual string writeToLog(){
		std::stringstream sstm;
		
		sstm << opIndex<<" "<<enumOpName[opId]<<" tid:"<<threadId<<" component:"<<component<<" id:"<<id<<" state:"<<state;
		return sstm.str();
	}

	const string& getComponent() const {
		return component;
	}

	void setComponent(const string& component) {
		this->component = component;
	}

	long getId() const {
		return id;
	}

	void setId(long id) {
		this->id = id;
	}

	const string& getState() const {
		return state;
	}

	void setState(const string& state) {
		this->state = state;
	}
	
};

struct Attach_QOperation:public Operation{
protected:
	long queueId;
public:
	Attach_QOperation(int index, int opId, int threadId, long queueId):Operation(index, opId, threadId){
		this->queueId=queueId;
	}
	Attach_QOperation(int index, int opId, int threadId, int taskId, long queueId):Operation(index, opId, threadId, taskId){
		this->queueId=queueId;
	}
	
	virtual Attach_QOperation * getClone(){
		Attach_QOperation *opClone = new Attach_QOperation(*this);
		return opClone;
	}
	void dumpOpInfo(){
		Operation::dumpOpInfo();
		cerr<<"Queue Id : "<<queueId<<endl;
	}

	virtual string writeToLogWithFormatting(){
		std::stringstream sstm;
		for(int i=1; i<threadId; i++)
			sstm << "\t";
		sstm << opIndex<<" "<<enumOpName[opId]<<" tid:"<<threadId<<" queue:"<<queueId;
		return sstm.str();
	}

	virtual string writeToLog(){
		std::stringstream sstm;
		
		sstm << opIndex<<" "<<enumOpName[opId]<<" tid:"<<threadId<<" queue:"<<queueId;
		return sstm.str();
	}

	long getQueueId() const {
		return queueId;
	}

	void setQueueId(long queueId) {
		this->queueId = queueId;
	}
};

struct PostOperation:public Operation{
protected:
	int postedTaskId;
	int destThreadId;
	int beginOpIndex;		// Index of begin operation of posted task!
	int endOpIndex;			// Index of End operation of posted task!
public:
	PostOperation(int index, int opId, int threadId, int postedTaskId, int destThreadId):Operation(index, opId, threadId){
		this->postedTaskId=postedTaskId;
		this->destThreadId=destThreadId;
		this->beginOpIndex=-1;
		this->endOpIndex=-1;
	}
	PostOperation(int index, int opId, int threadId, int taskId, int postedTaskId, int destThreadId):Operation(index, opId, threadId, taskId){
		this->postedTaskId=postedTaskId;
		this->destThreadId=destThreadId;
		this->beginOpIndex=-1;
		this->endOpIndex=-1;
	}

	virtual PostOperation * getClone(){
		PostOperation *opClone = new PostOperation(*this);
		return opClone;
	}
	void dumpOpInfo(){
		Operation::dumpOpInfo();
		cerr<<"Posted Task Id : "<<postedTaskId<<endl;
		cerr<<"Destination Thread Id : "<<destThreadId<<endl;
		cerr<<"Index of Begin operation of posted task : "<<beginOpIndex<<endl;
		cerr<<"Index of End operation of posted task : "<<endOpIndex<<endl;
	}

	virtual string writeToLogWithFormatting(){
		std::stringstream sstm;
		for(int i=1; i<threadId; i++)
			sstm << "\t";
		sstm << opIndex<<" "<<enumOpName[opId]<<" src:"<<threadId<<" msg:"<<postedTaskId<<" dest:"<<destThreadId;
		return sstm.str();
	}

	virtual string writeToLog(){
		std::stringstream sstm;
		
		sstm << opIndex<<" "<<enumOpName[opId]<<" src:"<<threadId<<" msg:"<<postedTaskId<<" dest:"<<destThreadId;
		return sstm.str();
	}

	int getDestThreadId() const {
		return destThreadId;
	}

	void setDestThreadId(int destThreadId) {
		this->destThreadId = destThreadId;
	}

	int getPostedTaskId() const {
		return postedTaskId;
	}

	void setPostedTaskId(int postedTaskId) {
		this->postedTaskId = postedTaskId;
	}

	int getBeginOpIndex() const {
		return beginOpIndex;
	}

	void setBeginOpIndex(int beginOpIndex) {
		this->beginOpIndex = beginOpIndex;
	}

	int getEndOpIndex() const {
		return endOpIndex;
	}

	void setEndOpIndex(int endOpIndex) {
		this->endOpIndex = endOpIndex;
	}
};

struct WindowFocusOperation:public Operation{
protected:
	long windowHash;
public:
	WindowFocusOperation(int index, int opId, int threadId, long windowHash):Operation(index, opId, threadId){
		this->windowHash=windowHash;
	}
	WindowFocusOperation(int index, int opId, int threadId, int taskId, long windowHash):Operation(index, opId, threadId, taskId){
		this->windowHash=windowHash;
	}
	
	virtual WindowFocusOperation * getClone(){
		WindowFocusOperation *opClone = new WindowFocusOperation(*this);
		return opClone;
	}
	void dumpOpInfo(){
		Operation::dumpOpInfo();
		cerr<<"WindowHash : "<<windowHash<<endl;
	}

	virtual string writeToLogWithFormatting(){
		std::stringstream sstm;
		for(int i=1; i<threadId; i++)
			sstm << "\t";
		sstm << opIndex<<" "<<enumOpName[opId]<<" tid:"<<threadId<<" WindowHash:"<<windowHash;
		return sstm.str();
	}

	virtual string writeToLog(){
		std::stringstream sstm;
		
		sstm << opIndex<<" "<<enumOpName[opId]<<" tid:"<<threadId<<" WindowHash:"<<windowHash;
		return sstm.str();
	}

	long getWindowHash() const {
		return windowHash;
	}

	void setWindowHash(long queueId) {
		this->windowHash = queueId;
	}
};

struct EnableEventOperation:public Operation{
protected:
	long view;
	int event;
public:
	EnableEventOperation(int index, int opId, int threadId, long view, int event):Operation(index, opId, threadId){
		this->view=view;
		this->event=event;
	}

	EnableEventOperation(int index, int opId, int threadId, int taskId, long view, int event):Operation(index, opId, threadId, taskId){
		this->view=view;
		this->event=event;
	}


	virtual EnableEventOperation * getClone(){
		EnableEventOperation *opClone = new EnableEventOperation(*this);
		return opClone;
	}

	virtual void dumpOpInfo(){
		Operation::dumpOpInfo();
		cerr<<"view : "<<view<<endl;
		cerr<<"event : "<<event<<endl;
	}

	virtual string writeToLogWithFormatting(){
		std::stringstream sstm;
		for(int i=1; i<threadId; i++)
			sstm << "\t";
		sstm << opIndex<<" "<<enumOpName[opId]<<" tid:"<<threadId<<" view:"<<view<<" event:"<<event;
		return sstm.str();
	}

	virtual string writeToLog(){
		std::stringstream sstm;
		
		sstm << opIndex<<" "<<enumOpName[opId]<<" tid:"<<threadId<<" view:"<<view<<" event:"<<event;
		return sstm.str();
	}

	int getEvent() const {
		return event;
	}

	void setEvent(int event) {
		this->event = event;
	}

	long getView() const {
		return view;
	}

	void setView(long view) {
		this->view = view;
	}
};

struct TriggerBroadcastOperation:public Operation{
protected:
	string action;
	long component;
	int intent;
	int onRecLater;
	string state;
public:
	TriggerBroadcastOperation(int index, int opId, int threadId, string action, long component, int intent, int onRecLater, string state):Operation(index, opId, threadId){
		this->action=action;
		this->component=component;
		this->intent=intent;
		this->onRecLater=onRecLater;
		this->state=state;
	}

	TriggerBroadcastOperation(int index, int opId, int threadId, int taskId, string action, long component, int intent, int onRecLater, string state):Operation(index, opId, threadId, taskId){
		this->action=action;
		this->component=component;
		this->intent=intent;
		this->onRecLater=onRecLater;
		this->state=state;
	}


	virtual TriggerBroadcastOperation * getClone(){
		TriggerBroadcastOperation *opClone = new TriggerBroadcastOperation(*this);
		return opClone;
	}

	virtual void dumpOpInfo(){
		Operation::dumpOpInfo();
		cerr<<"action : "<<action<<endl;
		cerr<<"component : "<<component<<endl;
		cerr<<"intent : "<<intent<<endl;
		cerr<<"onRecLater : "<<onRecLater<<endl;
		cerr<<"state : "<<state<<endl;
	}

	virtual string writeToLogWithFormatting(){
		std::stringstream sstm;
		for(int i=1; i<threadId; i++)
			sstm << "\t";
		sstm << opIndex<<" "<<enumOpName[opId]<<" tid:"<<threadId<<" action:"<<action<<" component:"<<component<<" intent:"<<intent<<" onRecLater:"<<onRecLater<<" state:"<<state;
		return sstm.str();
	}

	virtual string writeToLog(){
		std::stringstream sstm;
		
		sstm << opIndex<<" "<<enumOpName[opId]<<" tid:"<<threadId<<" action:"<<action<<" component:"<<component<<" intent:"<<intent<<" onRecLater:"<<onRecLater<<" state:"<<state;
		return sstm.str();
	}

	int getIntent() const {
		return intent;
	}

	void setIntent(int intent) {
		this->intent = intent;
	}

	long getOnRecLater() const {
		return onRecLater;
	}

	void setOnRecLater(int onRecLater) {
		this->onRecLater = onRecLater;
	}

	const string& getState() const {
		return state;
	}

	void setState(const string& state) {
		this->state = state;
	}

	const string& getAction() const {
		return action;
	}

	void setAction(const string& action) {
		this->action = action;
	}

	long getComponent() const {
		return component;
	}

	void setComponent(long component) {
		this->component = component;
	}
};

}
#endif /* ENTITIES_H_ */
