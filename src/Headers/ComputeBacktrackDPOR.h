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

#ifndef COMPUTEBACKTRACKDPOR_H_
#define COMPUTEBACKTRACKDPOR_H_


#include <VectorClock.h>

using namespace std;
using namespace emdpor_bookKeeper;
using namespace emdpor_Entities;
using namespace dpor_vc;

namespace dpor_backtrack{

inline int getState(int traceIndex){return (traceIndex-1);}

inline Operation * post(int taskId){
	if(taskId==-1){ cerr<<"backtrack::fn:task(), input operation was not executed inside a task!!"<<endl; throw error;}
	return getOperation(getParentPostIndex(taskId));
}

/*
 * returns a pair of 1) vector of threadIds. 2) index in trace : backtracking point.
 */
pair< set<int>, int> computeBacktrack(int i, Operation* opJ){		// should satisfy i<j
	//	Debug(cerr<<"fn:computeBacktrack() : begin "<<endl);
	set<int> resultSet;
	if(i==-1 || opJ == NULL){
		cerr<<"Error : in fn:computeBacktrackDPOR, invalid arguments passed!"<<endl;
		throw error;
		return make_pair(resultSet,i);
	}
	Operation * op1 = getOperation(i);
	Operation * op2 = opJ;

	i = op1->getOpIndex();
	int threadIdOfI = op1->getThreadId();

	Thread threadState = getThreadState(getState(i), op2->getThreadId());


	if(isThreadEnabled(getState(i), op2->getThreadId(), threadState) && isTaskExecutable(op2->getTaskId(), threadState)){
		Debug(cerr<<"Inserting thread id : "<<op2->getThreadId()<<endl);
		resultSet.insert(op2->getThreadId());
	}


	set<int> happenedBeforeThreadIds = findTaskExecutableHappenedBeforeThread(getState(i), op2->getOpIndex(), op2->getThreadId(), threadIdOfI);

	for(set<int>::iterator sit = happenedBeforeThreadIds.begin(); sit != happenedBeforeThreadIds.end(); ++sit)
		resultSet.insert(*sit);

	return make_pair(resultSet,i);
}


}


#endif /* COMPUTEBACKTRACKDPOR_H_ */
