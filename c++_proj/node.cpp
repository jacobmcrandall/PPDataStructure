#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <atomic>
#include <cstdint>
#define MAX_TOTAL_MASK 15
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

    node* ptr()
    {
      uintptr_t intPtr = reinterpret_cast<std::uintptr_t>(this);
      return reinterpret_cast<node*>(intPtr & (UINTPTR_MAX ^ MAX_TOTAL_MASK));
    }

    int getVal()
    {
      return this->ptr()->val;
    }

    void setVal(int val)
    {
      this->ptr()->val = val;
    }
};

//Can be up to 3 bits (max value, 7) change the 30 to another number to change bit amount
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
  //this might be really bad but I think it's okay
  value = value % MAX_TAG_MASK;
  auto intPtr = reinterpret_cast<std::uintptr_t>(nodePtr);
  intPtr = (intPtr & (UINTPTR_MAX ^ MAX_TAG_MASK)) | (value << 1);
  return reinterpret_cast<node*>(intPtr);
}

//To properly set you must reset your object pointer
//ie.) n = n->incrementTag;
node* incrementTag(node* nodePtr)
{
  return setTag(nodePtr,getTag(nodePtr) + 1);
}

bool getDeleteFlag(node* nodePtr)
{
  uintptr_t intPtr = reinterpret_cast<std::uintptr_t>(nodePtr);
  return intPtr & 1; //1 as last bit
}

//To properly set you must reset your object pointer
//ie.) n = n->setDeleteFlag(true);
node* setDeleteFlag(node* nodePtr,bool value)
{
  auto intPtr = reinterpret_cast<std::uintptr_t>(nodePtr);
  intPtr = (intPtr & (UINTPTR_MAX ^ 1)) | (value);
  return reinterpret_cast<node*>(intPtr);
}

node* flagAndTag(node* nodePtr, bool flag, int tag)
{
  return setTag(setDeleteFlag(nodePtr,flag),tag);
}

node* newNode()
{
  void* ptr;
  posix_memalign(&ptr, 32,sizeof(node)*4);
  return reinterpret_cast<node*>(ptr);
}

//LOCAL NODE STUFF

//Begin localNode (per thread)
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
