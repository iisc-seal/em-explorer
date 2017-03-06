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

#ifndef DEBUG_H_
#define DEBUG_H_

#define DEBUG_ENABLED 0                     //set this to 1 for detailed debug info, else 0
#define EXPLORED_FILE_WRITING_ENABLED 1     //set this to 1 for explored traces to be printed to file, else 0
#define FREQUENT_REPORTING_ENABLED 1        //set this to 1 for very minimal stats to be printed periodically as traces are being explored, else 0
#define DEBUG_CLOCK_ENBABLED 0              //set this to 1 to print vector clock timestamps, else 0
#define SELECTIVE_DEBUG_ENABLED 0           //set this to 1 for toned down debug info, else 0

#define Debug(x) do { \
  if (DEBUG_ENABLED) { x; } \
} while (0)

#define Write(x) do { \
  if (EXPLORED_FILE_WRITING_ENABLED) { x; } \
} while (0)

#define Report(x) do { \
  if (FREQUENT_REPORTING_ENABLED) { x; } \
} while (0)

#define DumpClock(x) do { \
  if (DEBUG_CLOCK_ENBABLED) { x; } \
} while (0)

#define DebugSelective(x) do { \
  if (SELECTIVE_DEBUG_ENABLED) { x; } \
} while (0)

#endif /* DEBUG_H_ */
