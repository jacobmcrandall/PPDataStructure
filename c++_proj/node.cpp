#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <atomic>
#include <cstdint>
//Max bits we use for our tag mask represented as a nubmer + one extra bit for the deleted flag
#define MAX_TOTAL_MASK 15
//Max bits we use for our tag mask represented as a number
#define MAX_TAG_MASK 7

class node
{
  public:
    std::atomic<node*> next;
    int val;

    node()
    {
      val = 0;
      next = NULL;
    }

    //B/c we are storing values in the end bits of the pointer we want to still be able to access a clean pointer so we have a mask that "wipes" these bits - we access this whenever we actually have to go to the next ptr
    node* ptr()
    {
      uintptr_t intPtr = reinterpret_cast<std::uintptr_t>(this);
      return reinterpret_cast<node*>(intPtr & (UINTPTR_MAX ^ MAX_TOTAL_MASK));
    }

    //Similarly to the above, when accessing ourself or any pointer we need to use the clean version
    //This is just shorthand for that
    int getVal()
    {
      return this->ptr()->val;
    }

    //Same as for getVal
    void setVal(int val)
    {
      this->ptr()->val = val;
    }
};

//Get the tag by masking our pointer and getting the last 3 bits (...____XXX_)
int getTag(node* nodePtr)
{
  uintptr_t intPtr = reinterpret_cast<std::uintptr_t>(nodePtr);
  return (intPtr & MAX_TAG_MASK) >> 1; //3 = (000...110)
}

//To properly set you must reset your object pointer
//ie.) n = n->setTag(5);
//Can be up to 3 bits (max value, 15) change the 30 to another number to change bit amount
node* setTag(node* nodePtr,int value)
{
  //This is not ideal but necessary as we only have X bits, we cannot set values higher than that amount
  value = value % MAX_TAG_MASK;
  auto intPtr = reinterpret_cast<std::uintptr_t>(nodePtr);
  //Resave the value by masking over the bits
  intPtr = (intPtr & (UINTPTR_MAX ^ MAX_TAG_MASK)) | (value << 1);
  return reinterpret_cast<node*>(intPtr);
}

//To properly set you must reset your object pointer
//ie.) n = n->incrementTag;
//Shorthand for setTag(+1)
node* incrementTag(node* nodePtr)
{
  return setTag(nodePtr,getTag(nodePtr) + 1);
}

//Similar to the above a simple mask to get the value of the last bit
bool getDeleteFlag(node* nodePtr)
{
  uintptr_t intPtr = reinterpret_cast<std::uintptr_t>(nodePtr);
  return intPtr & 1; //1 as last bit
}

//To properly set you must reset your object pointer
//ie.) n = n->setDeleteFlag(true);
//Similarly to the above we mask and set the value of the end bit
node* setDeleteFlag(node* nodePtr,bool value)
{
  auto intPtr = reinterpret_cast<std::uintptr_t>(nodePtr);
  intPtr = (intPtr & (UINTPTR_MAX ^ 1)) | (value);
  return reinterpret_cast<node*>(intPtr);
}

//Shorthand for setTag and setDeleteFlag
//We then also return whatever that value is
//This is great for doing shorter (text length) CAS'
node* flagAndTag(node* nodePtr, bool flag, int tag)
{
  return setTag(setDeleteFlag(nodePtr,flag),tag);
}

//This really could just return new node and be okay
//The idea is that if we need to save some on cache size or anything we can change how we do our
//memory allocation (maybe we even have a custom malloc)
node* newNode()
{
  void* ptr;
  posix_memalign(&ptr, 32,sizeof(node)*4);
  return reinterpret_cast<node*>(ptr);
}

//LOCAL NODE STUFF

//Begin localNode (per thread lists)
//Essentially this is just a simple linked list
class localNode
{
  public:
    node* data;
    localNode* next;

  localNode(node* newData)
  {
    data = newData;
    next = NULL;
  }
};

class localList
{
  public:
    localNode *head;

  localList()
  {
    head = new localNode(NULL);
  }

  void add(node* addData)
  {
    localNode* temp;

    temp = head->next;
    head->next = new localNode(addData);
    head->next->next = temp;
  }

  //Return the item if you found it else return NULL
  node* remove()
  {
    localNode *removedNode;
    node* retVal;
    removedNode = head->next;

    if(removedNode == NULL || removedNode->data == NULL || removedNode->data->ptr() == NULL) {return NULL;}

    retVal = removedNode->data;
    head->next = head->next->next;
    retVal = flagAndTag(retVal->ptr(), false, 0);
    retVal->ptr()->next = NULL;
    delete removedNode;
    return retVal;
  }
};
