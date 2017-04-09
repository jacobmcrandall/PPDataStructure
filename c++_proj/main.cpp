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

//Generate an array of randoms
int* setupRandoms(int numRandoms, double percentEnq)
{
  srand ( time(NULL) ); // seed random
  int* randomNums = new int[numRandoms];

  for(int i = 0; i<numRandoms; i++)
  {
    double randVal = (double)rand()/RAND_MAX;
    if(randVal < percentEnq)
      randomNums[i] = ENQ;
    else
      randomNums[i] = DEQ;
  }
  return randomNums;
}

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

void *threadWork(void *vargp)
{
    int currJob = jobsDone++;
    long threadID = (long)vargp;
    node *tempNode;

    while(currJob < numJobs)
    {
        if(randoms[currJob] == ENQ)
        {
          //Allocate new node if no nodes exist
          tempNode = localLists[threadID]->remove();
          if(tempNode == NULL)//list empty
          {
            tempNode = newNode();
          }
          q.add(tempNode,rand() % 100);
        }
        else
        {
          q.remove(localLists[threadID]);
        }
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
    // //Initialization
    numThreads = atoi(argv[1]);
    numJobs = atoi(argv[2]);
    percentEnq = atof(argv[3]);
    numPrepopulate = atoi(argv[4]);
    pthread_t threads[numThreads];

    //Make threads joinable
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    randoms = setupRandoms(numJobs, percentEnq);
    localLists = setupNodes(numThreads, numJobs);

    auto start = chrono::steady_clock::now();

    //Start work
    for(int i = 0; i < numThreads; i++)
    {
      pthread_create(&threads[i], &attr, threadWork, (void *)i );
    }

    //Join and wait for other threads to finish
    for(int i = 0; i < numThreads; i++)
      pthread_join(threads[i], NULL);

    auto end = chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    cout << "Execution time for "<< numThreads <<" threads was " << diff.count() << " ms. \n";

    // q.printQ();

    exit(0);
}
