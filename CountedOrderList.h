#ifndef __JCOUNTEDORDERLIST__
#define __JCOUNTEDORDERLIST__

#include <stdint.h>

#include "DLList.h"
#include "Logger.h"

namespace OrderBook {
template <typename NODE>
class CountedOrderList : public DLList<NODE> {
public:
   CountedOrderList()
   : level_quantity_(0)
   {}
   virtual ~CountedOrderList() {}

   void addNode(NODE *input) {
      level_quantity_ += input->order_qty_;
      DLList<NODE>::addNode(input);
   }

   void removeNode(NODE *input) {
      level_quantity_ -= input->order_qty_;
      DLList<NODE>::removeNode(input);
   }

   void changeNodeQuantity(NODE *input, uint32_t new_quantity) {
      level_quantity_ -= input->order_qty_;
      level_quantity_ += new_quantity;
      input->order_qty_ = new_quantity;
   }

   void clearLevel() {
      NODE *head = DLList<NODE>::getHead();
      while (head != DLList<NODE>::getTail()) {
         removeNode(head);
         head = head->previous_;
      }
      removeNode(head);
   }

   void printLevel(char tag, char *&buffer, int &index, int &max_buffer) {
      NODE *tail = DLList<NODE>::getTail();
      while (tail != DLList<NODE>::getHead()) {
         // Need interim so I know len of level print
         // char level[50];
         // sprintf(level, "%c %u ", tag, tail->order_qty_);
         // safeCopyToBuffer(buffer, level, index, max_buffer);

         if (index + 50 > max_buffer) growBuffer(buffer, max_buffer);
         index += sprintf(&buffer[index], "%c %u ", tag, tail->order_qty_);

         tail = tail->next_;
      }
      // Need interim so I know len of level print
      // char level[50];
      // sprintf(level, "%c %u ", tag, tail->order_qty_);
      // safeCopyToBuffer(buffer, level, index, max_buffer);

      if (index + 50 > max_buffer) growBuffer(buffer, max_buffer);
      index += sprintf(&buffer[index], "%c %u ", tag, tail->order_qty_);
   }

   uint32_t getQuantity() const { return level_quantity_; }

private:
   uint32_t level_quantity_;
};

} // namespace

#endif
