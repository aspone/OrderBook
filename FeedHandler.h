#ifndef __JFEEDHANDLER__
#define __JFEEDHANDLER__
#include <string>
#include <vector>

#include "HRTimer.h"
#include "LagHistogram.h"
#include "OrderBook.h"
#include "MessageParser.h"

#ifdef PROFILE
   #define STARTPROFILE() { timer_.start(); }
   #define STOPPROFILE(y) { y.add(timer_.stop()); }
#else
   #define STARTPROFILE()
   #define STOPPROFILE(y)
#endif

namespace OrderBook {
//-----------------------------------------------------------------------------
template<typename ORDERIDTYPE, typename ORDERTYPE>
class FeedHandler {
public:
   // Accept compiler ctor / dtor on default compile
#ifdef PROFILE
   // NOTE: Profile information contains both parsing and orderbook operations.
   FeedHandler()
   : order_book_()
   , parser_()
   , timer_()
   , add_("AddOrder")
   , modify_("ModifyOrder")
   , remove_("RemoveOrder")
   , trade_("Trade")
   , midquote_("MidQuote Print")
   , book_print_("Book Print")
   {}

   ~FeedHandler() {
      // Explicitly call stopLogger to block and get logger_ to flush before perf
      // printing...  coupling on C (A -> B -> C), but this is the profile build
      // so I'm not going to punch a hole in the OrderBook's abstraction.  Would
      // promote logger_ to singleton much like ErrorTracker if it were production
      // use necessary to block with flush here on ordering.
      order_book_.getLoggerReference().stopLogger();

      add_.print();
      modify_.print();
      remove_.print();
      trade_.print();
      midquote_.print();
      book_print_.print();
   }
#endif

   //------------------------------------------------------------
   void processMessage(char *line) {
#ifdef DEBUG
      uint32_t len = strlen(line);
      char orig_msg[len];
      strncpy(orig_msg, line, strlen(line));
#endif
      MessageType mt = parser_.getMessageType(line);
      bool valid_message = false;
      if (mt == eMT_Unknown) {
         FHErrorTracker::instance()->corruptMessage();
         return;
      }
      else if (mt == eMT_Trade) {
         STARTPROFILE();
         TradeMessage tm;
         parser_.parseTrade(line, tm);
         if (tm.trade_price_ != 0) {
            valid_message = true;
            DB("%s", orig_msg);
            order_book_.handleTrade(tm);
         }
         STOPPROFILE(trade_);
      }
      else {
         // Poked hole in memory model to avoid temporary stack-based ORDERTYPE
         // and copy ctor onto heap object.  Premature optimization out the
         // wazoo on this one, but "MOST OPTIMIZED CODE POSSIBLE" (within the
         // time constraint of the time I have to work on this) meant this nasty
         // split.  All "delete" calls are within OrderBook except for on bad
         // messages, and new / delete calls are kept tight here to attempt to
         // keep flow clear.  As mentioned in documentation - I'd be curious to
         // see how this performs with a ref counted shared_ptr from boost.
         STARTPROFILE();
         ORDERTYPE *ole = new ORDERTYPE();
         parser_.parseOrder(line, *ole);
         if (ole->order_side_ == eS_Unknown) {
            delete ole;
         }
         else {
            switch (mt) {
               case eMT_Add:
                  order_book_.addOrder(ole);
                  STOPPROFILE(add_);
                  valid_message = true;
                  break;
               case eMT_Modify:
                  order_book_.modifyOrder(ole);
                  STOPPROFILE(modify_);
                  valid_message = true;
                  break;
               case eMT_Remove:
                  order_book_.removeOrder(ole);
                  STOPPROFILE(remove_);
                  valid_message = true;
                  break;
               default:
                  fprintf(stderr, "Unknown order type hit in switch.  This should not happen.\n");
                  delete ole;
                  FAILASSERT();
                  break;
            }
         }
      }
      if (valid_message) {
#ifdef DEBUG
         DB("%s", orig_msg);
         order_book_.printBook();
#endif
         STARTPROFILE();
         order_book_.printMidpoint();
         STOPPROFILE(midquote_);
      }
   }

   void printCurrentOrderBook() {
   // Bail immediately as we're printing every message in DEBUG mode
#ifdef DEBUG
      return;
#endif
      STARTPROFILE()
      order_book_.printBook();
      STOPPROFILE(book_print_);
   }

private:
   OrderBook<ORDERIDTYPE, ORDERTYPE> order_book_;
   MessageParser parser_;

#ifdef PROFILE
   HRTimer timer_;
   LagHistogram add_;
   LagHistogram modify_;
   LagHistogram remove_;
   LagHistogram trade_;
   LagHistogram midquote_;
   LagHistogram book_print_;
#endif
};

} // namespace

#endif
