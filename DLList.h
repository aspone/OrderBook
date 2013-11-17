#ifndef __JDLLIST__
#define __JDLLIST__

#include <stdio.h>

namespace OrderBook {
//-----------------------------------------------------------------------------
template <typename NODE>
class DLList {
public:
   DLList()
   : head_(0)
   , tail_(0)
   {}

   virtual ~DLList() { }

   void addNode(NODE *input) {
      if (head_ == 0) {
         head_ = input;
         tail_ = head_;
         return;
      }

      head_->next_ = input;
      input->previous_ = head_;
      head_ = input;
   }

   void removeNode(NODE *target) {
      // Skip self and delete if straddled
      if (target->previous_ != 0 && target->next_ != 0) {
         target->previous_->next_ = target->next_;
         target->next_->previous_ = target->previous_;
      }
      // Edge - tail deleted
      else if (target == tail_ && target->next_ != 0) {
         tail_ = target->next_;
         tail_->previous_ = 0;
      }
      // Edge - head deleted
      else if (target == head_ && target->previous_ != 0) {
         head_ = target->previous_;
         head_->next_ = 0;
      }
      // Edge - single element list
      else {
         head_ = 0;
         tail_ = 0;
         return;
      }
   }

   NODE *getHead() const { return head_; }
   NODE *getTail() const { return tail_; }

#ifdef DEBUG
   void printList() {
      NODE *head = tail_->previous_;
      while (head != tail_) {
         fprintf(stderr, "NODE: ");
         head->printSelf();
         head = head->previous_;
      }
      fprintf(stderr, "TAIL NODE: ");
      head->printSelf();
   }
#endif

private:
   NODE *head_;
   NODE *tail_;
};

} // namespace

#endif
