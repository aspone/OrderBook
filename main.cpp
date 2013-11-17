#include <sched.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <fstream>

#include "FeedHandler.h"
#include "FHErrorTracker.h"
#include "HRTimer.h"
#include "LagHistogram.h"
#include "Utilities.h"

using namespace OrderBook;
//-----------------------------------------------------------------------------
int main(int argc, char **argv) {
   if (argc == 1) {
      std::cout << "Usage: OrderBookProcessor <filename>" << std::endl;
      return -1;
   }

   FHErrorTracker::instance()->init();

/* Performance is actually better without SCHED_RR... hm.  Not the 1st time
   I've seen this.
   struct sched_param sp;
   sp.sched_priority = sched_get_priority_max(SCHED_RR);
   if (sched_setscheduler(0, SCHED_RR, &sp) == -1) {
      std::cout << "Error setting scheduler to SCHED_RR.  Please run with " <<
         "permissions for realtime scheduling.  Aborting." << std::endl;
      return -1;
   }
*/

   FeedHandler<uint32_t, OrderLevelEntry> feed;
   const std::string filename(argv[1]);

   FILE *pFile;
   try {
      pFile = fopen(filename.c_str(), "r");
      if (pFile == NULL) {
         throw;
      }
   }
   catch (...) {
      std::cout << "Error occured opening file: " <<
         filename <<
         ".  Please check the file and try again." << std::endl;
      return -1;
   }

   uint32_t counter = 0;
   size_t len;
   char *buffer = NULL;
   while (!feof (pFile)) {
      while (1) {
         ssize_t read = getline(&buffer, &len, pFile);
         if (read == -1) break;

         feed.processMessage(buffer);

         if (buffer) free(buffer);
         buffer = 0;

         ++counter;
         if (counter % 10 == 0) {
            feed.printCurrentOrderBook();
         }
      }
   }
   fclose(pFile);
   
   FHErrorTracker::instance()->printStatistics();
   return 0;
}
