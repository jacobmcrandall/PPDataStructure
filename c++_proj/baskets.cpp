#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <atomic>
#include "node.cpp"
#define MAX_HOPS 3

class BasketsQueue
{
  private:
    std::atomic<node*> head;
    std::atomic<node*> tail;

  public:
    BasketsQueue()
    {
      node * sentinel = newNode();
      sentinel->setVal(0);
      sentinel->ptr()->next = flagAndTag(NULL, false, 0);
      head = tail = sentinel;
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
            nd->next.store(flagAndTag(NULL, false, getTag(tail) + 2));

            if(tail->ptr()->next.compare_exchange_strong(next, flagAndTag(nd, false, getTag(tail)+1) ))
            {
              this->tail.compare_exchange_strong(tail, flagAndTag(nd, false, getTag(tail)+1));
              return true;
            }
            next = tail->ptr()->next.load();
            while((getTag(next) == getTag(tail)+1) && !getDeleteFlag(next))
            {
              // printf("TODO BACKOFF_SCHEME HERE\n");
              nd->next.store(next);
              if(tail->ptr()->next.compare_exchange_strong(next, flagAndTag(nd, false, getTag(tail) + 1)))
                return true;
              next = tail->ptr()->next.load();
            }
          }
          else
          {
            while(next->ptr()->next.load()->ptr() != NULL && this->tail.load() == tail)
            {
              next = next->ptr()->next.load();
            }
            this->tail.compare_exchange_strong(tail, flagAndTag(next, 0 , getTag(tail) + 1));
          }
        }
      }
    }

    node* remove()
    {
      node *head, *tail, *next, *iter, *ret;
      int hops = 0;
      while(true)
      {
        head = this->head.load();
        tail = this->tail.load();
        next = head->ptr()->next.load();
        if(head == this->head.load())
        {
          if(head->ptr() == tail->ptr())
          {
            if(next->ptr() == NULL)
                return NULL; //empty
            while(next->ptr()->next.load()->ptr() != NULL && this->tail.load() == tail)
            {
              next = next->ptr()->next.load();
            }
            this->tail.compare_exchange_strong(tail, flagAndTag(next->ptr(), false , getTag(tail) + 1));
          }
          else
          {
            iter = head;
            hops = 0;
            while((getDeleteFlag(next) && iter->ptr() != tail->ptr()) && this->head.load() == head)
            {
              iter = next;
              next = iter->ptr()->next.load();
              hops++;
            }

            if(this->head != head)
              continue;
            else if(iter->ptr() == tail->ptr())
              freeChain(head,iter);
            else
            {
              ret = next->ptr();
              if(iter->ptr()->next.compare_exchange_strong(next, flagAndTag(next->ptr(),true,getTag(next) + 1)))
              {
                if(hops >= MAX_HOPS)
                  freeChain(head,next);
                return ret;
              }
              // printf("TODO BACKOFF_SCHEME HERE\n");
            }
          }
        }
      }
    }

    void freeChain(node* head, node* newHead)
    {
      node* next;
      int count = 0;
      if(this->head.compare_exchange_strong(head, flagAndTag(newHead->ptr(), 0 , getTag(head) + 1)));
      {
        //Might need to investiage why we had to add head->ptr() != NULL
        while (head->ptr() != NULL && head->ptr() != newHead->ptr())
        {
          count++;
          next = head->ptr()->next.load();
          // reclaim_node(head->ptr());
          // delete head->ptr();
          head = next;
        }
      }
    }

    //Ay this isn't thread safe - just for simple testing / debugging
    void printQ()
    {
      if(this->head.load()->ptr() == this->tail.load()->ptr())
      {
        std::cout << "Q is empty\n";
        return;
      }
      std::cout << "head : " << this->head.load() << "\n";
      std::cout << "tail : " << this->tail.load() << "\n";
      node *curr = this->head.load()->ptr()->next.load();

      while (true)
      {
        if(curr->ptr() == NULL) {break;}
        if(!getDeleteFlag(curr))
        {
          std::cout << curr->getVal() << " -> ";
        }

        if(curr == this->tail.load()) {break;}
        curr = curr->ptr()->next;
      }
      std::cout << "\n";
    }
};
