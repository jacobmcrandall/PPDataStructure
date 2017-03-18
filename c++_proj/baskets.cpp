#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <atomic>
#include "node.cpp"

//TODO: Implement baskets queue linked list / necessary features for that
class BasketsQueue
{
  private:
    std::atomic<node*> head;

  public:
    BasketsQueue()
    {
      head = NULL;
    }

    void add(node* n, int val)
    {
      //TODO
    }

    node* remove()
    {
      //TODO
      return NULL;
    }
};
