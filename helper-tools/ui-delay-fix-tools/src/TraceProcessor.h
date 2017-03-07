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

#ifndef TRACEPROCESSOR_H_
#define TRACEPROCESSOR_H_

#include <stdlib.h>
#include <iostream>
#include <string>
#include <map>
#include <list>
#include <vector>

#include "Entities.h"
#include "Dependence.h"

using namespace std;
using namespace emdpor_Entities;

namespace traceProcessorHelper{

//taskId, opIndex
map<int, int > postMap;

// beginIndex, postIndex
map<int,int> beginToPosMap;

//index,op
map<int,Operation *> operationMap;

//map<int,string> trace;
vector<string> traceList;


ExplicitDependencies depObj, orgDepObj;
set<int> depValueSet;
//int movedFrom=0;
//int movedTo=0;

//done set - contains msg-id for fixed delay-post
set<int> done;

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

string getKeyValue(string key, string str){
//	int index=str.find(key);
//	if(index==-1)
//		cerr<<"trying to find invalid key : "<<key<<" in string : "<<str<<endl;

	return str.substr(str.find(":")+1);
}

bool isNumber(char c){
	if((c>='0' && c<='9') || c=='+' || c=='-')
		return true;
	return false;
}

int getInt(string str){
	//	Debug(cerr<<"Inside getInt, input string is : "<<str<<endl);
	int length = str.length();
	char st[12];
	int i=0;
	for( ;i<length; i++){
		st[i]=str[i];
	}
	st[i]='\0';
	const char *c = st;
	//	int ti = atoi(c);
	//	Debug(cerr<<"leaving getInt, output is : "<<ti<<endl);
	return atoi(c);
}

int getLong(string str){
	//	Debug(cerr<<"Inside getLong(), input string is : "<<str<<endl);
	int length = str.length();
	char st[24];
	int i=0;
	for( ;i<length; i++){
		st[i]=str[i];
	}
	st[i]='\0';
	const char *c = st;
	//return atol(c);
	return strtol(c,NULL,0);
}

//returns a pointer to newly created Operation object for each line of log processed, returns null for don't care Ops.

//int processLine(string str){
//
//	if((str.length()<=2) || !isNumber(str[0])){
//		return 0;
//	}
//
//	string indexString = getFirstToken(str, " ");
//	str = getLeftOverString(str, " ");
//	//	int index = std::atoi(indexString);
//	int index = getInt(indexString);
//	string operationName =  getFirstToken(str, " ");
//	str = getLeftOverString(str, " ");
//	string threadId = getKeyValue(/*"tid", */getFirstToken(str, " "));
//	int tid = getInt(threadId);
//	return tid;
//}



bool processDependence(string str){
	if(str.length() < 9)
		return false;
	int src = getInt(getKeyValue("src", getFirstToken(str, " ")));
	int dest = getInt(getKeyValue("dest", getLastToken(str, " ")));
	depObj.addDependency(src,dest);
	depValueSet.insert(src);
	depValueSet.insert(dest);
	return true;
}

Operation * processLine(string str){
	//	Debug(cerr<<"Inside processLine(), input string is : "<<str<<endl);
	if((str.length()<=2) || !isNumber(str[0])){
		//		Debug(cerr<<"Invalid String, returning NULL at 1"<<endl);
		if(!str.compare("START")){
			//trace[1]=str;
			traceList.push_back("START");
		}
		return NULL;
	}
	string save = str;
	Operation *retOp = NULL;
	string indexString = getFirstToken(str, " ");
	str = getLeftOverString(str, " ");
	//	int index = std::atoi(indexString);
	int index = getInt(indexString);
	//trace[index]=save;
	traceList.push_back(save);
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


Operation * processTraceLine(string str){
	//	Debug(cerr<<"Inside processLine(), input string is : "<<str<<endl);
	if((str.length()<=2) || !isNumber(str[0])){
		//		Debug(cerr<<"Invalid String, returning NULL at 1"<<endl);
		return NULL;
	}
	string save = str;
	Operation *retOp = NULL;
	string indexString = getFirstToken(str, " ");
	str = getLeftOverString(str, " ");
	//	int index = std::atoi(indexString);
	int index = getInt(indexString);
	//trace[index]=save;
	//traceList.push_back(save);
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

void processOp(Operation *op){
	if(op==NULL)
		return;
	int opId = op->getOpId();
	int opIndex = op->getOpIndex();
	operationMap[opIndex]=op;
	PostOperation *pop;
	int temp1,temp2;
	map<int,int>::iterator it;
	Operation*  tempOp;
	switch(opId){
		case POST:
		case UI_POST:
		case DELAY_POST:
		case NATIVE_POST:
			pop = dynamic_cast<PostOperation *>(op);
			postMap[pop->getPostedTaskId()]=pop->getOpIndex();
			//cerr<<pop->getTaskId()<<endl;
			break;
		case BEGIN:
			//cerr<<"here1"<<endl;
			it = postMap.find(op->getTaskId());
			if(it==postMap.end()){
				cerr<<"couldn't find post op"<<endl;
				cerr<<"post op index" <<op->getOpIndex() <<" msg id : " << op->getTaskId()<<endl;
			}
			tempOp = operationMap.find(it->second)->second;
			//cerr<<"here2"<<endl;
			pop = dynamic_cast<PostOperation *>(tempOp);
			//cerr<<"here3"<<endl;
			pop->setBeginOpIndex(opIndex);
			beginToPosMap[opIndex] = pop->getOpIndex();
			break;
		default:break;
	}

}

void resetStructures(){
	cerr<<"resetStructures() beginning"<<endl;
	postMap.erase(postMap.begin(), postMap.end());
	beginToPosMap.erase(beginToPosMap.begin(), beginToPosMap.end());
	map<int,Operation*>::iterator it = operationMap.begin();
	
	while(!operationMap.empty()){
		Operation *op = operationMap.begin()->second;
		operationMap.erase(operationMap.begin());
		delete op;
	}
	cerr<<"trace size : "<<traceList.size()<<endl;
	for(int i=0; i<traceList.size(); ++i)
		cerr<<traceList.at(i)<<endl;

	for(int i=0; i<traceList.size(); ++i){
		Operation *op = processTraceLine(traceList.at(i));
		if(op!=NULL){
			processOp(op);
		}
	}
	cerr<<"resetStructures() ending"<<endl;
}

void fixSequenceIndices(){
	cerr<<"fixSequenceIndices :: begin"<<endl;
	vector<string> tempList = traceList;
	traceList.erase(traceList.begin(), traceList.end());
	int index=1;
	for(vector<string>::iterator it = tempList.begin(); it != tempList.end(); ++it,++index){
		string line = *it;
		line = getLeftOverString(line, " ");
		std::stringstream sstm;
		sstm << index << " " << line;
		traceList.push_back(sstm.str());
	}
	cerr<<"fixSequenceIndices :: end"<<endl;
}

int getChangedDepValue(int from, int to, int value){
	if(value > from && value < to)
		return value-1;
	else
		return value;
}

void fixDependences(int moveFrom, int moveTo){
	cerr<<"fixDependences :: begin"<<endl;
	map<int,set<int> > depMap = depObj.getSourceToTargetMap();
	ExplicitDependencies tempObj;
	for(map<int,set<int> >::iterator it = depMap.begin(); it != depMap.end(); ++it){
		set<int> dest = it->second;
		for(set<int>::iterator sit = dest.begin(); sit != dest.end(); ++sit){
			tempObj.addDependency(getChangedDepValue(moveFrom, moveTo, it->first), getChangedDepValue(moveFrom, moveTo, *sit));
		}
	}
	depObj = tempObj;
	cerr<<"fixDependences :: end"<<endl;
}

void fixTrace(){
	//orgDepObj = depObj;
	map<int,Operation*>::iterator it;
	for(it=operationMap.begin(); it != operationMap.end(); ++it){
		int index = it->first;
		Operation *op = it->second;
		//after fixing this delay post, put this into some kind of done set, so it doesn't get processesd again.
		//if(op->getOpId() == UI_POST){
		if(op->getOpId() == DELAY_POST){
			PostOperation *pop = dynamic_cast<PostOperation *>(op);
			
			if(done.find(pop->getPostedTaskId()) != done.end())
				continue;
			else
				done.insert(pop->getPostedTaskId());

			int beginIndex = pop->getBeginOpIndex();
			int destThreadId = pop->getDestThreadId();
			cerr<<"beginIndex : "<<beginIndex<<endl;

			//movedFrom = index;
			int previousPost=0; bool flag0=false;
			for(map<int,Operation*>::reverse_iterator iit =operationMap.rbegin(); iit!=operationMap.rend(); ++iit){
				if(index==iit->first){
					flag0=true;
					continue;
				}
				if(flag0){
					Operation *oop = iit->second;
					if(oop->getOpId()==POST || oop->getOpId()==UI_POST || oop->getOpId()==NATIVE_POST || oop->getOpId()==DELAY_POST){
						PostOperation *postOp = dynamic_cast<PostOperation*>(oop);
						if(postOp->getDestThreadId()==destThreadId){
							previousPost=oop->getOpIndex();
							break;
						}
					}
				}
			}

			
			//map<int,Operation*>::iterator iit=operationMap.find(index);
			int previousBeginIndex=0; bool flag=false;
			for(map<int,Operation*>::reverse_iterator iit =operationMap.rbegin(); iit!=operationMap.rend(); ++iit){
				if(beginIndex==iit->first){
					flag=true;
					continue;
				}
				if(flag){
					Operation *oop = iit->second;
					if(oop->getOpId()==BEGIN && oop->getThreadId()==destThreadId){
						previousBeginIndex=oop->getOpIndex();
						break;
					}
				}
			}
			if(previousBeginIndex==0)
				continue;

			cerr<<"previousBeginIndex : "<<previousBeginIndex<<endl;
			int previousPostIndex = beginToPosMap.find(previousBeginIndex)->second;
			cerr<<"previousPostIndex : "<<previousPostIndex<<endl;

			if(previousPost == previousPostIndex)
				continue;
			//movedTo=previousPostIndex+1;
			string opString = traceList.at(previousPostIndex);
			cerr<<"index : "<<index<<endl;
			string delayString = traceList.at(index-1);
			cerr<<"opString : "<<opString<<endl;
			cerr<<"delayString : "<<delayString<<endl;

			vector<string>::iterator vit = traceList.begin();
			for(int ii=1; ii<=previousPostIndex; ++ii,vit++);
			traceList.insert(vit, delayString);
			
			vit = traceList.begin();
			for(int ii=1; ii<index; ++ii,vit++);
			traceList.erase(vit);
			
			fixSequenceIndices();
			fixDependences(index,previousPostIndex+1);
			//break;
			resetStructures();
			it = operationMap.begin();
		}
	}
}

void fixTrace2(){
	map<int,Operation*>::iterator it;
	for(it=operationMap.begin(); it != operationMap.end(); ++it){
		int index = it->first;
		//cerr<<"index : "<<index<<endl;
		Operation *op = it->second;
		//after fixing this delay post, put this into some kind of done set, so it doesn't get processesd again.
		if(op->getOpId() == DELAY_POST){
			PostOperation *pop = dynamic_cast<PostOperation *>(op);
			
			if(done.find(pop->getPostedTaskId()) != done.end())
				continue;
			else
				done.insert(pop->getPostedTaskId());

			//movedFrom = index;
			int beginIndex = pop->getBeginOpIndex();
			int destThreadId = pop->getDestThreadId();
			cerr<<"beginIndex : "<<beginIndex<<endl;
			map<int,Operation*>::iterator iit=operationMap.find(beginIndex);
			int nextBeginIndex=0;
			for( ++iit ; iit!= operationMap.end(); ++iit){
				Operation *oop = iit->second;
				if(oop->getOpId()==BEGIN && oop->getThreadId()==destThreadId){
					nextBeginIndex = iit->first;
					break;
				}
			}
			cerr<<"nextBeginIndex : "<<nextBeginIndex<<endl;
			int nextPostIndex = beginToPosMap.find(nextBeginIndex)->second;
			cerr<<"nextPostIndex : "<<nextPostIndex<<endl;
			//movedTo=nextPostIndex;
			string opString = traceList.at(nextPostIndex-1);
			cerr<<"index : "<<index<<endl;
			string delayString = traceList.at(index-1);
			cerr<<"opString : "<<opString<<endl;
			cerr<<"delayString : "<<delayString<<endl;
			vector<string>::iterator vit = traceList.begin();
			for(int ii=1; ii<nextPostIndex; ++ii,vit++);
			traceList.insert(vit, delayString);
			vit = traceList.begin();
			for(int ii=1; ii<index; ++ii,vit++);
			traceList.erase(vit);
			fixSequenceIndices();
			fixDependences(index,nextPostIndex);
			resetStructures();
			it = operationMap.begin();
			//break;
		}
	}
}

void dumpTrace(){
	cerr<<"dumping trace below --- "<<endl<<endl;

	for(int i =0; i<traceList.size(); ++i)
		cerr<<traceList.at(i)<<endl;
}

void dumpDep(){
	cerr<<endl<<"dumping original dependences below ---"<<endl<<endl;
	orgDepObj.dumpSourceToTargetMap();

	cerr<<endl<<"dumping dependences below ---"<<endl<<endl;
	depObj.dumpSourceToTargetMap();
}

void writeTraceToFile(string fileName){
	std::ofstream traceFile;
	string line;
	traceFile.open(fileName.c_str());
	try{
		if (traceFile.is_open()){
			for(int i=0; i<traceList.size(); ++i)
				traceFile << traceList.at(i) << endl;

			traceFile << endl << "End of trace!!";

			traceFile.close();
		}

	}
	catch(exception &ex){
		cerr<<ex.what()<<endl;
		if(traceFile.is_open())
			traceFile.close();
	}
}

void writeDepToFile(string fileName){
	std::ofstream depFile;
	string line;
	depFile.open(fileName.c_str());
	try{
		if (depFile.is_open()){
			map<int,set<int> > depMap = depObj.getSourceToTargetMap();
			for(map<int,set<int> >::iterator it = depMap.begin(); it != depMap.end(); ++it){
				set<int> dest = it->second;
				for(set<int>::iterator sit = dest.begin(); sit != dest.end(); ++sit){
					depFile << "src:" << it->first << " dest:" << *sit << endl;	
				}
			}
			depFile.close();
		}

	}
	catch(exception &ex){
		cerr<<ex.what()<<endl;
		if(depFile.is_open())
			depFile.close();
	}
}

void saveOrgDep(){
	orgDepObj = depObj;
}

void dumpDoneSet(){
	cerr<<endl<<"done set ---, size : "<<done.size()<<endl;
	for(set<int>::iterator it = done.begin(); it!=done.end(); ++it){
		cerr<<"msg:"<<*it<<endl;
	}
}

}
#endif /* TRACEPROCESSOR_H_ */
