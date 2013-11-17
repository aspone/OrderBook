#ifndef __JHRTIMER__
#define __JHRTIMER__

#include <stdint.h>
#include <unistd.h>

#include "Utilities.h"

namespace OrderBook {
//-----------------------------------------------------------------------------
class HRTimer {
public:
   HRTimer()
   : start_(0) {
      int64_t start = RDTSC();
      usleep(250000);
      int64_t end = RDTSC();
      int64_t duration = end - start;

      uint32_t ticks_sec = duration * 4;
      fprintf(stderr, "Ticks/sec: %u.  MHz: %f\n", ticks_sec, ticks_sec / 1000000.0f);
      hz_ = ticks_sec;
   }

   inline void start() {
      start_ = RDTSC();
   }

   inline uint64_t stop() {
      if (UNLIKELY(start_ == 0)) return 0;
      uint64_t nano = ((RDTSC() - start_) * 1000000000) / hz_;
      start_ = 0;
      return nano;
   }
private:
   //-----------------------------------------------------------------------------
   inline int64_t RDTSC() {
      static uint32_t hi, lo;
      __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
      return ( (uint64_t)lo) | (((uint64_t)hi) << 32);
   }
   uint64_t start_;
   uint64_t hz_;
};

} // namespace

#endif
