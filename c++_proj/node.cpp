#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <atomic>

class node
{
  private:

  public:
    std::atomic<node*> next;
    int val;
    node()
    {
      val = 0;
      next = NULL;
    }
};
