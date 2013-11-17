#ifndef __JMESSAGEPARSER__
#define __JMESSAGEPARSER__

#include <stdint.h>

#include <boost/spirit/include/qi_uint.hpp>
#include <boost/spirit/include/qi_real.hpp>

#include "FHErrorTracker.h"
#include "Utilities.h"

using namespace boost::spirit;

namespace OrderBook {
enum ParseStatus {
   ePS_Good,
   ePS_CorruptMessage,
   ePS_BadQuantity,
   ePS_BadPrice,
   ePS_BadID,
   ePS_GenericBadValue
};

//-----------------------------------------------------------------------------
class MessageParser {
public:
   MessageParser()
   {}
   // Not intended for inheritance
   ~MessageParser() {}

   inline MessageType getMessageType(char *message);

   inline void parseOrder(char *tk_msg, OrderLevelEntry &ole);
   inline void parseTrade(char *tk_msg, TradeMessage &tm);

private:
   inline ParseStatus tokenizeAndConvertToUint(char *tk_msg, uint32_t &dest);
   inline ParseStatus tokenizeAndConvertToDouble(char *tk_msg, double &dest);

   inline void reportStatus(ParseStatus status);
   inline void failOrderParse(OrderLevelEntry &ole, ParseStatus status);
   inline void failTradeParse(TradeMessage &tm, ParseStatus status);
};

//-----------------------------------------------------------------------------
inline MessageType MessageParser::getMessageType(char *tk_msg) {
   uint32_t len = strlen(tk_msg);
   if (len == 0 || len > MESSAGELENMAX) {
      FHErrorTracker::instance()->corruptMessage();
      return eMT_Unknown;
   }

   tk_msg = strtok(tk_msg, ",");
   switch(tk_msg[0]) {
      case 'A':
         return eMT_Add;
         break;
      case 'M':
         return eMT_Modify;
         break;
      case 'X':
         return eMT_Remove;
         break;
      case 'T':
         return eMT_Trade;
         break;
      default:
         return eMT_Unknown;
         break;
   }
}

//-----------------------------------------------------------------------------
inline ParseStatus MessageParser::tokenizeAndConvertToUint(char *tk_msg, uint32_t &dest) {
   tk_msg = strtok(NULL, ",");
   if (tk_msg == NULL) {
      return ePS_CorruptMessage;
   }
   // Prevent boost qi from rolling on 1st bit
   else if (tk_msg[0] == '-') {
      return ePS_GenericBadValue;
   }
   else if (!qi::parse(tk_msg, &tk_msg[strlen(tk_msg)], uint_, dest)) {
      return ePS_GenericBadValue;
   }
   return ePS_Good;
}

//-----------------------------------------------------------------------------
inline ParseStatus MessageParser::tokenizeAndConvertToDouble(char *tk_msg, double &dest) {
   tk_msg = strtok(NULL, ",");
   if (tk_msg == NULL) {
      return ePS_CorruptMessage;
   }
   // Prevent boost qi from rolling on 1st bit
   else if (tk_msg[0] == '-') {
      return ePS_GenericBadValue;
   }
   else if (!qi::parse(tk_msg, &tk_msg[strlen(tk_msg)], double_, dest)) {
      return ePS_GenericBadValue;
   }
   return ePS_Good;
}

//-----------------------------------------------------------------------------
inline void MessageParser::reportStatus(ParseStatus status) {
   switch (status) {
      case ePS_Good:
         FHErrorTracker::instance()->goodMessage();
         break;
      case ePS_CorruptMessage:
         FHErrorTracker::instance()->corruptMessage();
         break;
      case ePS_BadQuantity:
         FHErrorTracker::instance()->invalidQuantity();
         break;
      case ePS_BadPrice:
         FHErrorTracker::instance()->invalidPrice();
         break;
      case ePS_BadID:
         FHErrorTracker::instance()->invalidID();
         break;
      default:
         fprintf(stderr, "Unknown parsing error occurred.  Skipping report of error.\n");
         FAILASSERT();
         break;
   }
}

//-----------------------------------------------------------------------------
inline void MessageParser::failOrderParse(OrderLevelEntry &ole, ParseStatus status) {
   // Flag as failed parse
   ole.order_side_ = eS_Unknown;
   return reportStatus(status);
}

//-----------------------------------------------------------------------------
inline void MessageParser::parseOrder(char *tk_msg, OrderLevelEntry &ole) {
   ParseStatus result = tokenizeAndConvertToUint(tk_msg, ole.order_id_);
   if (result != ePS_Good) {
      if (result == ePS_CorruptMessage)
         return failOrderParse(ole, result);
      return failOrderParse(ole, ePS_BadID);
   }

   tk_msg = strtok(NULL, ",");
   if (tk_msg == NULL) {
      return failOrderParse(ole, ePS_CorruptMessage);
   }
   else {
      switch(tk_msg[0]) {
         case 'B':
            ole.order_side_ = eS_Buy;
            break;
         case 'S':
            ole.order_side_ = eS_Sell;
            break;
         default:
            return failOrderParse(ole, ePS_CorruptMessage);
            break;
      }
   }

   result = tokenizeAndConvertToUint(tk_msg, ole.order_qty_);
   if (result != ePS_Good) {
      if (result == ePS_CorruptMessage)
         return failOrderParse(ole, result);
      return failOrderParse(ole, ePS_BadQuantity);
   }
   if (ole.order_qty_ == 0) return failOrderParse(ole, ePS_BadQuantity);

   double price = 0;
   result = tokenizeAndConvertToDouble(tk_msg, price);
   if (result != ePS_Good) {
      if (result == ePS_CorruptMessage)
         return failOrderParse(ole, result);
      return failOrderParse(ole, ePS_BadPrice);
   }
   if (static_cast<unsigned long long>(price * 100) == 0) return failOrderParse(ole, ePS_BadPrice);
   // technically not supporting prices up to double here.
   if (price > ULLONG_MAX) {
      return failOrderParse(ole, ePS_BadPrice);
   }

   // Bail if not 0-2 decimals of precision
   if ((double)(price * 100) - static_cast<unsigned long long>(price * 100) != 0) {
      return failOrderParse(ole, ePS_BadPrice);
   }
   ole.order_price_ = static_cast<unsigned long long>(price * 100);

   FHErrorTracker::instance()->goodMessage();
}

//-----------------------------------------------------------------------------
inline void MessageParser::failTradeParse(TradeMessage &tm, ParseStatus status) {
   tm.trade_qty_ = 0;
   tm.trade_price_ = 0;
   reportStatus(status);
}

//-----------------------------------------------------------------------------
inline void MessageParser::parseTrade(char *tk_msg, TradeMessage &tm) {
   ParseStatus result = tokenizeAndConvertToUint(tk_msg, tm.trade_qty_);
   if (result != ePS_Good) {
      if (result == ePS_CorruptMessage)
         return failTradeParse(tm, result);;
      return failTradeParse(tm, ePS_BadQuantity);
   }
   if (tm.trade_qty_ == 0) return failTradeParse(tm, ePS_BadPrice);

   double price;
   result = tokenizeAndConvertToDouble(tk_msg, price);
   if (result != ePS_Good) {
      if (result == ePS_CorruptMessage)
         return failTradeParse(tm, result);
      return failTradeParse(tm, ePS_BadPrice);
   }
   // technically not supporting prices up to double here.
   if (price > ULLONG_MAX) {
      return failTradeParse(tm, ePS_BadPrice);
   }

   // Bail if not 0-2 decimals of precision
   if ((double)(price * 100) - static_cast<unsigned long long>(price * 100) != 0) {
      return failTradeParse(tm, ePS_BadPrice);
   }
   if (static_cast<unsigned long long>(price * 100) == 0) return failTradeParse(tm, ePS_BadPrice);

   tm.trade_price_ = static_cast<unsigned long long>(price * 100);
   if (tm.trade_price_ > MAXPRICE - 1) {
      DB("Price: %llu.  Max: %d\n", tm.trade_price_, MAXPRICE - 1);
      return failTradeParse(tm, ePS_BadPrice);
   }

   FHErrorTracker::instance()->goodMessage();
}

} // namespace

#endif
