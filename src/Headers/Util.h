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


#ifndef UTIL_H_
#define UTIL_H_

#include<set>

using namespace std;

set<int> setDiff(const set<int> & set1, const set<int> & set2){
	set<int> result;
	set_difference(set1.begin(), set1.end(), set2.begin(), set2.end(), inserter(result, result.end()));
	return result;
}

set<int> setIntersection(set<int> set1, set<int> set2){
	set<int> result;
	set_intersection(set1.begin(), set1.end(), set2.begin(), set2.end(), inserter(result, result.begin()));
	return result;
}

template <typename T>
set<T> getSetForVector(vector<T> v){
	set<T> resultSet;
	typename vector<T>::iterator it;
	for(it = v.begin(); it != v.end(); ++it)
		resultSet.insert(*it);
	return resultSet;
}

template <typename T>
vector<T> getVectorForSet(set<T> s){
	vector<T> resultVector;
	typename set<T>::iterator it;
	for(it = s.begin(); it != s.end(); ++it)
		resultVector.push_back(*it);
	return resultVector;
}

struct FailSave{
public:
	int failIndex;
	vector<int> seq;
	bool initialized;

	void init(int failIndex, vector<int> seq){
		this->failIndex = failIndex;
		this->initialized = true;
		for(int i=0; i<=failIndex; ++i){
			this->seq.push_back(seq.at(i));
		}
	}
	bool isEqual(int failIndex, vector<int> seq){
		if(this->failIndex != failIndex)
			return false;
		for(int i = 0; i <= failIndex; i++){
			if( this->seq.at(i) != seq.at(i) )
				return false;
		}
		return true;
	}
	bool isInitialized(){return initialized;}
	void clean(){
		initialized=false;
		failIndex=-1;
		seq.erase(seq.begin(),seq.end());
	}
};

class myexception: public exception
{
  virtual const char* what() const throw()
  {
    return "TraceExplorer Error";
  }
} error;

void cleanFile(char *filename){
	ofstream myfile;
	myfile.open(filename);
	myfile.close();
}

void cleanFile(string filename){
	ofstream myfile;
	myfile.open(filename.c_str());
	myfile.close();
}

template <typename T>
void dumpSet(const set<T> & s){
	if(s.empty()){
		cerr<<"empty"<<endl;
		return;
	}
	typename set<T>::iterator it;
	for(it = s.begin(); it != s.end(); ++it)
		cerr<<*it<<endl;
}

template <typename T>
void dumpVector(const vector<T> & s){
	if(s.empty()){
		cerr<<"empty"<<endl;
		return;
	}
	typename vector<T>::iterator it;
	for(it = s.begin(); it != s.end(); ++it)
		cerr<<*it<<endl;
}

void initializeArray(int array[], int size, int value=0){
	for(int i = 0; i < size; i++){
		array[i] = value;
	}
}

#endif /* UTIL_H_ */
