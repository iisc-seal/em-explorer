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
 * Defines structures required to maintain various kinds of sets such as
 * done set, backtracking set etc. at each state visited by the sequence being explored.
 */


#ifndef SETS_H_
#define SETS_H_

#include <set>

using namespace std;

struct Sets{

private:
	set<int> backtrackingSet;
	set<int> sleepSet; //we do not maintain/use sleep sets for this branch
	set<int> doneSet;
	bool dporRunStatus;
public:

	Sets(){
		dporRunStatus = false;
	}

	const set<int>& getImmutableBacktrackingSet() const {
		return backtrackingSet;
	}

	const set<int>& getImmutableDoneSet() const {
		return doneSet;
	}

	const set<int>& getImmutableSleepSet() const {
		return sleepSet;
	}

	set<int>& getMutableBacktrackingSet() {
		return backtrackingSet;
	}

	set<int>& getMutableDoneSet() {
		return doneSet;
	}

	set<int>& getMutableSleepSet() {
		return sleepSet;
	}

	bool hasDporRun() const {
		return dporRunStatus;
	}

	void setDporRunStatus(bool dporRunStatus) {
		this->dporRunStatus = dporRunStatus;
	}
	
	void dumpSets(){
		cerr<<"Backtracking Set : ";
		for(set<int>::iterator vit = backtrackingSet.begin(); vit != backtrackingSet.end(); ++vit)
			cerr<<*vit<<", ";
		cerr<<endl<<"Done Set         : ";
		for(set<int>::iterator vit = doneSet.begin(); vit != doneSet.end(); ++vit)
			cerr<<*vit<<", ";
		cerr<<endl<<"Sleep Set        : ";
		for(set<int>::iterator vit = sleepSet.begin(); vit != sleepSet.end(); ++vit)
			cerr<<*vit<<", ";
		cerr<<endl;
	}

};


#endif /* SETS_H_ */
