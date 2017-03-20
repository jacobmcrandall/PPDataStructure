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

    //Can be up to 4 bits (max value, 15) change the 30 to another number to change bit amount
    int getTag()
    {
      uintptr_t intPtr = reinterpret_cast<std::uintptr_t>(this);
      return (intPtr & 30) >> 1; //30 = (000...11110)
    }

    //To properly set you must reset your object pointer
    //ie.) n = n->setTag(5);
    //Can be up to 4 bits (max value, 15) change the 30 to another number to change bit amount
    node* setTag(int value)
    {
      auto intPtr = reinterpret_cast<std::uintptr_t>(this);
      intPtr = (intPtr & (UINTPTR_MAX ^ 30)) | (value << 1);
      return reinterpret_cast<node*>(intPtr);
    }

    //To properly set you must reset your object pointer
    //ie.) n = n->incrementTag;
    node* incrementTag()
    {
      return setTag(this->getTag() + 1);
    }

    bool getDeleteFlag()
    {
      uintptr_t intPtr = reinterpret_cast<std::uintptr_t>(this);
      return intPtr & 1; //1 as last bit
    }

    //To properly set you must reset your object pointer
    //ie.) n = n->setDeleteFlag(true);
    node* setDeleteFlag(bool value)
    {
      auto intPtr = reinterpret_cast<std::uintptr_t>(this);
      intPtr = (intPtr & (UINTPTR_MAX ^ 1)) | value;
      return reinterpret_cast<node*>(intPtr);
    }
};
