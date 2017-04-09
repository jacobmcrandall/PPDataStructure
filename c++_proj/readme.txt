Compile with "make"

Run with ./main

./main :
	Argument 1 : Number of threads : int (0 - INT_MAX)
	Argument 2 : Number of jobs : int (0 - INT_MAX)
		   - Jobs are total, not per thread
	Argument 3 : Percentage of enqueues : decimal (0 - 1)
		   - The remaining percent will be the percentage of dequeues
		   - This is not guaranteed but rather an estimate utilizing rand
	Argument 4 : Number of nodes each thread gets pre-populated with : boolean (0 - INT_MAX)
			 - This is how many nodes we pre-assign to a list before doing work
			 - If we are only measuring speed of the add / removes prepopulating improves performance
			 - We are still safe even if this value is 0 as a threadsafe malloc is used in the case that we link pthread (we do).
