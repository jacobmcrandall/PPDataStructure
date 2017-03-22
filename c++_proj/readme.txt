Compile with "make"

Run with ./main

./main : 
	Argument 1 : Number of threads : int (0 - INT_MAX)
	Argument 2 : Number of jobs : int (0 - INT_MAX)
		   - Jobs are total, not per thread
	Argument 3 : Percentage of enqueues : decimal (0 - 1)
		   - The remaining percent will be the percentage of dequeues
		   - This is not gauranteed but rather an estimate utilizing rand
