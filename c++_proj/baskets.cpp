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
      //Initialize a sentinel node as both the head and tail representing the "empty Q"
      node* sentinel = newNode();
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
        //Ensure the tail isn't lagging
        if(tail == this->tail.load())
        {
          //If we next is still the last element in the Q (nothing else was added)
          if(next->ptr() == NULL)
          {
            //Set nodes next to a NULL pointer
            nd->next.store(flagAndTag(NULL, false, getTag(tail) + 2));

            //Do a CAS on the tail, if succeeded we inserted a new node with no issues
            if(tail->ptr()->next.compare_exchange_strong(next, flagAndTag(nd, false, getTag(tail)+1) ))
            {
              this->tail.compare_exchange_strong(tail, flagAndTag(nd, false, getTag(tail)+1));
              return true;
            }
            //If we get here we failed our insert, at this point we need to insert into a basket
            next = tail->ptr()->next.load();
            //Loop to get to the correct insert point then insert, if we fail an insert we just retry the insert into the basket
            while((getTag(next) == getTag(tail)+1) && !getDeleteFlag(next))
            {
              nd->next.store(next);
              if(tail->ptr()->next.compare_exchange_strong(next, flagAndTag(nd, false, getTag(tail) + 1)))
                return true;
              next = tail->ptr()->next.load();
            }
          }
          else
          {
            //We are lagging we need to update our tail, find the correct tail, update and reset
            while(next->ptr()->next.load()->ptr() != NULL && this->tail.load() == tail)
            {
              next = next->ptr()->next.load();
            }
            this->tail.compare_exchange_strong(tail, flagAndTag(next, 0 , getTag(tail) + 1));
          }
        }
      }
    }

    node* remove(localList* removeToList)
    {
      node *head, *tail, *next, *iter, *ret;
      int hops = 0;
      while(true)
      {
        head = this->head.load();
        tail = this->tail.load();
        next = head->ptr()->next.load();
        //Ensure head is still the value we loaded
        if(head == this->head.load())
        {
          //We cannot remove from an empty Q
          if(head->ptr() == tail->ptr())
          {
            if(next->ptr() == NULL)
                return NULL; //empty
            //Some item was inserted before we could return null
            while(next->ptr()->next.load()->ptr() != NULL && this->tail.load() == tail)
            {
              next = next->ptr()->next.load();
            }
            //Update tail and continue
            this->tail.compare_exchange_strong(tail, flagAndTag(next->ptr(), false , getTag(tail) + 1));
          }
          else
          {
            iter = head;
            hops = 0;
            //Find a chain of logically deleted items and continue until we don't
            while((getDeleteFlag(next) && iter->ptr() != tail->ptr()) && this->head.load() == head)
            {
              iter = next;
              next = iter->ptr()->next.load();
              hops++;
            }

            if(this->head != head)
              continue;
            //We hit the tail and have an "empty Q" (all logically deleted)
            else if(iter->ptr() == tail->ptr())
              freeChain(head,iter,removeToList);
            else
            {
              ret = next->ptr();
              //Try to exchange which will remove
              if(iter->ptr()->next.compare_exchange_strong(next, flagAndTag(next->ptr(),true,getTag(next) + 1)))
              {
                //Free if we found an excessively long chain
                if(hops >= MAX_HOPS)
                  freeChain(head,next,removeToList);
                return ret;
              }
              //If implementing backoff scheme do it here
            }
          }
        }
      }
    }

    //Free a chain of logically deleted nodes
    void freeChain(node* head, node* newHead, localList* removeToList)
    {
      node* next;
      //Jump into the free chain method, this makes sure we really only have one value in this if for a chain instance
      if(this->head.compare_exchange_strong(head, flagAndTag(newHead->ptr(), 0 , getTag(head) + 1)));
      {
        //While we have more items in the chain continually free logically deleted items
        while (head->ptr() != NULL && head->ptr() != newHead->ptr())
        {
          next = head->ptr()->next.load();
          removeToList->add(head->ptr()); //aka reclaimNode
          head = next;
        }
      }
    }

    //This isn't thread safe or part of the algorithm - just for simple testing / debugging
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
