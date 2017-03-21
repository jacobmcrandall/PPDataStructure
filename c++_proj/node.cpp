#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <atomic>
#include <cstdint>

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

    node* ptr()
    {
      uintptr_t intPtr = reinterpret_cast<std::uintptr_t>(this);
      return reinterpret_cast<node*>(intPtr & (~31));
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

//Can be up to 4 bits (max value, 15) change the 30 to another number to change bit amount
int getTag(node* nodePtr)
{
  uintptr_t intPtr = reinterpret_cast<std::uintptr_t>(nodePtr);
  return (intPtr & 30); //30 = (000...11110)
}

//To properly set you must reset your object pointer
//ie.) n = n->setTag(5);
//Can be up to 4 bits (max value, 15) change the 30 to another number to change bit amount
node* setTag(node* nodePtr,int value)
{
  auto intPtr = reinterpret_cast<std::uintptr_t>(nodePtr);
  intPtr = (intPtr & (UINTPTR_MAX ^ 30)) | (value);
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
