This folder consists of examples with a single queue and multiple threads. They
test various aspects of algorithm like, 
1. identifying and reordering diverging posts (trace1,trace2) 
2. scenarios involving both single threaded and cross thread dependences (trace4,trace5,trace6) 
3. handling scenarios that recursively need to reorder post operations (trace3) 
4. scenarios where backtrack failure is encountered (trace14)
5. scenarios involving multiple reads racing with a write and need special handling
   to explore all valid orderings between dependent transitions (trace10,trace11,trace12)
6. deadlock traces (trace13)
