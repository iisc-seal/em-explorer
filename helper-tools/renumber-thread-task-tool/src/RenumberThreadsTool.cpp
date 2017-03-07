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

#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <set>
#include <map>
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>

using namespace std;

set<int> tasks, threads;

bool isNumber(char c){
	if((c>='0' && c<='9') || c=='+' || c=='-')
		return true;
	return false;
}

string getFirstToken(string str, string delimiter){  //returns string upto delimiter
	string token;
	int index = str.find(delimiter);
	if(index!=-1)
		token = str.substr(0,index);
	else
		return str;
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

int getInt(string str){
	const char *c = str.c_str();
	return atoi(c);
}

string getKeyValue(string key, string str){
	return str.substr(str.find(":")+1);
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

void preProcessLine(string str){
	if((str.length()<=2) || !isNumber(str[0])){
		return;
	}
	string indexString = getFirstToken(str, " ");
	str = getLeftOverString(str, " ");
//	int index = getInt(indexString);
	string operationName =  getFirstToken(str, " ");
	str = getLeftOverString(str, " ");


	if(!operationName.compare("THREADINIT")){
		string threadId = getKeyValue("tid", getFirstToken(str, " "));
		int tid = getInt(threadId);
		threads.insert(tid);
	}
	else if(!operationName.compare("CALL")){			//CALL = begin_task
		string threadId = getKeyValue("tid", getFirstToken(str, " "));
//		int tid = getInt(threadId);
		str = getLastToken(str, " ");
		string msg = getKeyValue("msg", str);
		int taskId = getInt(msg);
		tasks.insert(taskId);
	}
}

class myexception: public exception
{
  virtual const char* what() const throw()
  {
    return "TraceExplorer Error";
  }
} error;

map<int,int> threadIdMap, taskIdMap;

void renumberIDs(){
	int i=1;
	for(set<int>::iterator it = threads.begin(); it != threads.end(); ++it,++i){
		threadIdMap[*it] = i;
	}
	i=1;
	for(set<int>::iterator it = tasks.begin(); it != tasks.end(); ++it,++i){
		taskIdMap[*it] = i;
	}
}

int getNewThreadId(int oldThreadId){
	map<int,int>::iterator it = threadIdMap.find(oldThreadId);
	if(it!=threadIdMap.end())
		return it->second;
	cerr<<"couldn't find thread Id in mapper, throwing! "<< oldThreadId <<endl;
	throw error;
}

int getNewTaskId(int oldTaskId){
	map<int,int>::iterator it = taskIdMap.find(oldTaskId);
	if(it!=taskIdMap.end())
		return it->second;
	cerr<<"couldn't find task Id in mapper, throwing! "<<oldTaskId<<endl;
	throw error;
}

static inline std::string &rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
        return s;
}

string postProcessLine(string str){
	if((str.length()<=8) || !isNumber(str[0])){
		return str+'\n';
	}
	string input = str;
	string indexString = getFirstToken(str, " ");
	str = getLeftOverString(str, " ");
//	int index = getInt(indexString);
	string operationName =  getFirstToken(str, " ");
	str = getLeftOverString(str, " ");
	string retStr = indexString + " " + operationName;

	if(!operationName.compare("FORK")){
		string parentThreadId = getKeyValue("par-tid", getFirstToken(str, " "));
		int parentTid = getInt(parentThreadId);
		string childThreadId = getKeyValue("child-tid", getLastToken(str, " "));
		int childTid = getInt(childThreadId);
		stringstream ss_par;
		ss_par << getNewThreadId(parentTid);
		retStr = retStr + " par-tid:" + ss_par.str();
		stringstream ss_child;
		ss_child << getNewThreadId(childTid);
		retStr = retStr + " child-tid:" + ss_child.str();
	}
	else if(!operationName.compare("NOTIFY")){
		string parentThreadId = getKeyValue("tid", getFirstToken(str, " "));
		int parentTid = getInt(parentThreadId);
		string childThreadId = getKeyValue("notifiedTid", getLastToken(str, " "));
		int childTid = getInt(childThreadId);
		stringstream ss_par;
		ss_par << getNewThreadId(parentTid);
		retStr = retStr + " tid:" + ss_par.str();
		stringstream ss_child;
		ss_child << getNewThreadId(childTid);
		retStr = retStr + " notifiedTid:" + ss_child.str();
	}
	else if(!operationName.compare("POST") || !operationName.compare("UI-POST") || !operationName.compare("NATIVE-POST") || !operationName.compare("DELAY-POST")){
		string threadId = getKeyValue("src", getFirstToken(str, " "));
		int tid = getInt(threadId);
		str = getLeftOverString(str, " ");
		stringstream ss;
		ss << getNewThreadId(tid);
		retStr = retStr + " src:" + ss.str();
		string postedTaskId = getKeyValue("msg", getFirstToken(str, " "));
		int tskid = getInt(postedTaskId);
		str = getLeftOverString(str, " ");
		stringstream ss1;
		ss1 << getNewTaskId(tskid);
		retStr = retStr + " msg:" + ss1.str();
		string destThreadId = getKeyValue("dest", getFirstToken(str, " "));
		int destTid = getInt(destThreadId);
		stringstream ss2;
		ss2 << getNewThreadId(destTid);
		retStr = retStr + " dest:" + ss2.str();
	}
	else if(!operationName.compare("CALL") || !operationName.compare("RET")){
		string threadId = getKeyValue("tid", getFirstToken(str, " "));
		int tid = getInt(threadId);
		str = getLeftOverString(str, " ");
		string msg = getKeyValue("msg", str);
		int taskId = getInt(msg);
		stringstream ss;
		ss << getNewThreadId(tid);
		stringstream ss1;
		ss1 << getNewTaskId(taskId);
		retStr = retStr + " tid:" + ss.str() + " msg:" + ss1.str();
	}
	else{
		string threadId = getKeyValue("tid", getFirstToken(str, " "));
		int tid = getInt(threadId);
		stringstream ss;
		ss << getNewThreadId(tid);
		str = getLeftOverString(str, " ");
		retStr = retStr + " tid:" + ss.str() + " " + str;
	}
	retStr=rtrim(retStr);
	return retStr+'\n';
}

int main() {
	string line;
	ifstream logFile ("input_trace.txt");
	ofstream myfile;
	myfile.open ("trace.txt");
	if (logFile.is_open() && myfile.is_open())
	{
		while ( getline (logFile,line) )
		{
			preProcessLine(line);
		}
		logFile.close();

		renumberIDs();

		logFile.open("input_trace.txt");
		while ( getline (logFile,line) )
		{
			cerr<<line<<endl;
			string str = postProcessLine(line);
			myfile<<str;
		}
		logFile.close();
		myfile.close();
		cout<<"program completed successfully  !!";
	}
	else
		cout<<"problem in opening the files!"<<endl;
}
