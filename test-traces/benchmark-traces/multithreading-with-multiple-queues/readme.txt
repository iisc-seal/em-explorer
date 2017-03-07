This folder consists of traces with multiple threads attached to event queues.
These examples test the actions of the algorithm reschedulePending(). Some of the
aspects tested by these traces:

1. need for post-to-post edges to not miss any partial orderings (trace1)
2. importance of reschedulePending (trace3,trace7)
3. features like divergingPosts, recursive FindTarget and other 
   things affecting single queue case too (trace4,trace5,trace6)
4. scenarios where backtrack failure is encountered (trace8)
5. deadlock traces (trace2)
