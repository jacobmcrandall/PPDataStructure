#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <limits.h>
#include <iostream>
#include <chrono>
#include "baskets.cpp"
using namespace std;
#define ENQ 0
#define DEQ 1

BasketsQueue q;
std::atomic<int> jobsDone;
int numJobs, numThreads, numPrepopulate;
double percentEnq;
int* randoms;
localList **localLists;

//Generate an array of randoms and return a pointer to the array
int* setupRandoms(int numRandoms, double percentEnq)
{
  srand ( time(NULL) ); // seed random
  int* randomNums = new int[numRandoms];

  for(int i = 0; i<numRandoms; i++)
  {
    double randVal = (double)rand()/RAND_MAX;
    //Percentages are not a gaurantee but rather guiding
    if(randVal < percentEnq)
      randomNums[i] = ENQ;
    else
      randomNums[i] = DEQ;
  }
  return randomNums;
}

//Set up a local noe list for each thread and return the pointer to the allocated array
//Add nodes to each local list up to some number
localList** setupNodes(int numThreads, int numJobs)
{
  localList** list = new localList*[numThreads];
  for (int i = 0; i < numThreads; i++)
  {
    list[i] = new localList[numJobs];
    for(int j = 0; j < numPrepopulate; j++)
    {
      list[i]->add(newNode());
    }
  }
  return list;
}

//The actual thread work, continually grab work as there is work to do
//(an enq or deq) then return
void *threadWork(void *vargp)
{
    //Jobs done is an atomic int and simple ++ or setting will happen atomically
    //That is this is translated to a fetch for us
    //B/c this is atomic too after claiming a job we are gauranteed it
    int currJob = jobsDone++;
    long threadID = (long)vargp;
    node *tempNode;

    while(currJob < numJobs)
    {
      //If the job you got is enq
        if(randoms[currJob] == ENQ)
        {
          //Grab some node if we can find one
          tempNode = localLists[threadID]->remove();
          //Allocate new node if no nodes exist
          if(tempNode == NULL)//list empty
          {
            tempNode = newNode();
          }
          //Add the node and some random value
          q.add(tempNode,rand() % 100);
        }
        else
        {
          //We remove from the queue and place the node into our local list
          q.remove(localLists[threadID]);
        }
        //claim next job
        currJob = jobsDone++;
    }
}

//Takes three arguments
// 1 - Number of threads
// 2 - Number of jobs (total, not per thread, each thread will do an equal number of jobs)
// 3 - Percent of jobs that are enq (0-1, rest are deqs)
// 4 - Number of nodes each local list should be pre-populated with
int main(int argc, char *argv[])
{
    //Initialization & grab arguments and save them
    numThreads = atoi(argv[1]);
    numJobs = atoi(argv[2]);
    percentEnq = atof(argv[3]);
    numPrepopulate = atoi(argv[4]);
    pthread_t threads[numThreads];

    //Make threads joinable
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    //Set up our dynamic arrays for both randoms and nodes, along with some pre-seeding
    //before we begin our work
    randoms = setupRandoms(numJobs, percentEnq);
    localLists = setupNodes(numThreads, numJobs);

    auto start = chrono::steady_clock::now();

    //Start work for each thread
    for(int i = 0; i < numThreads; i++)
    {
      pthread_create(&threads[i], &attr, threadWork, (void *)i );
    }

    //Join and wait for other threads to finish when one thread is done
    for(int i = 0; i < numThreads; i++)
      pthread_join(threads[i], NULL);

    auto end = chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    cout << "Execution time for "<< numThreads <<" threads was " << diff.count() << " ms. \n";

    //Can uncomment this if you want to see the ending values but printQ is not thread safe
    // q.printQ();

    exit(0);
}
