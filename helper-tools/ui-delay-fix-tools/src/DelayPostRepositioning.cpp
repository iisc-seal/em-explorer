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


#include <iostream>
#include <iterator>
#include <utility>
#include <fstream>
#include "TraceProcessor.h"

using namespace std;

int main(int argc, char* argv[])
{
	std::ifstream traceFile, depFile;
	string line = "input_trace.txt";
	traceFile.open(line.c_str());
	line = "input_dependence.txt";
	depFile.open(line.c_str());
	try{
		if (traceFile.is_open() && depFile.is_open())
		{
	
			while ( getline (traceFile,line) )
			{
				//cerr<<line<<endl;
				Operation *op = traceProcessorHelper::processLine(line);
				if(op!=NULL){
					traceProcessorHelper::processOp(op);
				}
			}

			while ( getline (depFile,line) )
			{
				//cerr<<line<<endl;
				if(!traceProcessorHelper::processDependence(line)){
					break;
				}
			}

			traceFile.close();
			depFile.close();
			traceProcessorHelper::saveOrgDep();

			traceProcessorHelper::fixTrace();
			//traceProcessorHelper::dumpTrace();
			//traceProcessorHelper::dumpDep();
			traceProcessorHelper::dumpDoneSet();
			traceProcessorHelper::writeTraceToFile("trace.txt");
			traceProcessorHelper::writeDepToFile("dependence.txt");
			cerr <<endl<< "Program terminated successfully...!"<<endl;
		}
	}
	catch(exception &ex){
		cerr<<ex.what()<<endl;

		if(traceFile.is_open())
			traceFile.close();
		else
			cerr << "Unable to open trace file"<<endl;

		if(depFile.is_open())
			depFile.close();
		else
			cerr << "Unable to open dependence file"<<endl;

		return 1;
	}
		
	return 0;
}
