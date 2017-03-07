This folder contains benchmark traces for various concurrency settings like pure multithreading,
multithreading with single queue, and with multiple queues. Apart from these high level categories,
you will find traces with varied nature testing various parts of our EM-DPOR algorithm. Please
consult the readme.txt within each of the top level folders to know the behaviour tested by the
traces of that folder. The traces seen here have been written in the trace language borrowed from 
the paper "Race Detection for Android Applications, PLDI 2014". 
Note that these are handcrafted traces and not generated using DroidRacer by exploring some 
Android app, unlike the traces reported in our paper "Partial Order Reduction for Event-driven 
Multi-threaded Programs", TACAS 2016.

To test EM-DPOR you can create your own traces by looking up these sample traces.
