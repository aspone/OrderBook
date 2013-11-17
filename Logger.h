#ifndef __JLOGGER__
#define __JLOGGER__

#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#define TRUE 1

class Logger {
public:
   //------------------------------------------------------------
   Logger()
   : exit_(false)
   , messages_available_(false)
   , thread_(0)
   , messages_()
   , mutex_()
   {}

   //------------------------------------------------------------
   ~Logger() {
      // Guard against it never being used or being stopped elsewhere
      if (thread_ == 0) return;

      exit_ = true;
      thread_->join();
      delete thread_;
      thread_ = 0;
   }

   //------------------------------------------------------------
   // provide option to manually block and let all logs flush on exit
   void stopLogger() {
      exit_ = true;
      thread_->join();
      delete thread_;
      thread_ = 0;
   }

   //------------------------------------------------------------
   void print(std::string msg) {
      // lazy init creation
      // Optimizeable.  For now, keep interface clean and hide temporal coupling
      // from users.
      if (thread_ == 0) {
         init();
      }
      // mutex protects against messing with messages_ while logging thread
      // is copying out to it's own queue
      std::lock_guard<std::mutex> lock(mutex_);
      messages_.push(msg);
      messages_available_ = true;
   }

   //------------------------------------------------------------
   void runLogger() {
      while (TRUE) {
         if (messages_available_ == true) {
            copyMessages();
            printMessages();
         }
         if (exit_ == true) {
            copyMessages();
            printMessages();
            return;
         }
      }
   }

private:
   //------------------------------------------------------------
   void copyMessages() {
      // Just copy them all out.  Deal with strcpy costs here in bulk.
      // Could optimize later if it becomes a problem and blocks enqueue
      std::lock_guard<std::mutex> lock(mutex_);
      while (messages_.size() != 0) {
         messages_to_print_.push(messages_.front());
         messages_.pop();
      }
      messages_available_ = false;
   }
   //------------------------------------------------------------
   void init() {
      thread_ = new std::thread(std::bind(&Logger::runLogger, this));
   }

   //------------------------------------------------------------
   void printMessages() {
      while (messages_to_print_.size() != 0) {
         fprintf(stderr, "%s", messages_to_print_.front().c_str());
         messages_to_print_.pop();
      }
   }

   std::atomic<bool> exit_;
   std::atomic<bool> messages_available_;
   std::thread *thread_;
   std::queue<std::string> messages_;
   std::queue<std::string> messages_to_print_;
   std::mutex mutex_;
};

#endif
