#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <atomic>
#include "node.cpp"

class BasketsQueue
{
  private:
    std::atomic<node*> head;
    std::atomic<node*> tail;

  public:
    BasketsQueue()
    {
      head = tail = flagAndTag(new node,0 ,0);
    }

    bool add(node* nd, int val)
    {
      node *tail, *next;
      nd->setVal(val);
      while(true)
      {
        tail = this->tail.load();
        next = tail->ptr()->next.load();
        if(tail == this->tail.load())
        {
          if(next->ptr() == NULL)
          {
            nd->next = flagAndTag(NULL, false, getTag(tail) + 2);
            if(tail->ptr()->next.compare_exchange_strong(next, flagAndTag(nd, false, getTag(tail)+1) ))
            {
              this->tail.compare_exchange_strong(tail, flagAndTag(nd, false, getTag(tail)+1));
              return true;
            }
            next = tail->ptr()->next;
            while((getTag(next) == getTag(tail)+1) && !getDeleteFlag(next))
            {
              //backoff scheme
              nd->next = next;
              if(tail->next.compare_exchange_strong(next, flagAndTag(nd, false, getTag(tail) + 1)))
                return true;
              next = tail->next;
            }
          }
          else
          {
            while(next->ptr()->next.load()->ptr() != NULL && this->tail.load() == tail)
            {
              next = next->ptr()->next;
            }
            this->tail.compare_exchange_strong(tail, flagAndTag(next, 0 , getTag(tail) + 1));
          }
        }
      }
    }

    node* remove()
    {
      //TODO
      return NULL;
    }

    //Ay this isn't thread safe - just for simple testing / debugging
    void printQ()
    {
      node *curr = this->head.load()->next;
      std::cout << "\n~~PRINTING~~\n\n";

      while (true)
      {
        std::cout << curr->getVal() << " -> ";
        if(curr == this->tail.load()) {break;}
        curr = curr->ptr()->next;
      }
      std::cout << "\n";
    }
};
