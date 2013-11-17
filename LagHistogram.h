#ifndef __LAGHISTOGRAM__
#define __LAGHISTOGRAM__

#include <limits.h>
#include <stdint.h>
#include <string>

#include <algorithm>
#include <string>
#include <vector>

class LagHistogram {
public:
   LagHistogram(std::string title, uint32_t sample_count = 0)
   : title_(title)
   , samples_()
   {
      if (sample_count > 0)
         samples_.reserve(sample_count);
   }

   void add(uint64_t input) {
      samples_.push_back(input);
   }

   void print() {
      if (samples_.size() == 0) {
         fprintf(stderr, "[%s] No valid samples for run.\n", title_.c_str());
         return;
      }

      std::sort(samples_.begin(), samples_.end());
      uint64_t sum = 0;

      fprintf(stderr, "\nPerformance results for [%s] (unit: nanoseconds)\n", title_.c_str());
      for(uint32_t i = 0; i < samples_.size(); ++i) {
         sum += samples_[i];
      }
      fprintf(stderr, "   %-10s %10lu\n", "Samples:", samples_.size());
      fprintf(stderr, "   %-10s %10lu\n", "Min:", samples_[0]);
      fprintf(stderr, "   %-10s %10lu\n", "Max:", samples_[samples_.size() - 1]);
      fprintf(stderr, "   %-10s %10lu\n", "Mean:", sum / samples_.size());
      fprintf(stderr, "   %-10s %10lu\n", "Median:", samples_[samples_.size() / 2]);
      if (samples_.size() > 10) {
         fprintf(stderr, "\n   %-20s\n", "[Percentiles]");
         fprintf(stderr, "   %-10s %10lu\n", "10th:", samples_[(samples_.size() / 10)]);
         fprintf(stderr, "   %-10s %10lu\n", "20th:", samples_[(samples_.size() / 10.) * 2]);
         fprintf(stderr, "   %-10s %10lu\n", "50th:", samples_[(samples_.size() / 10.) * 5]);
         fprintf(stderr, "   %-10s %10lu\n", "70th:", samples_[(samples_.size() / 10.) * 7]);
         fprintf(stderr, "   %-10s %10lu\n", "90th:", samples_[(samples_.size() / 10.) * 9]);
      }
      if (samples_.size() > 100) {
         fprintf(stderr, "   %-10s %10lu\n", "95th:", samples_[(samples_.size() / 100.) * 95]);
         fprintf(stderr, "   %-10s %10lu\n", "99th:", samples_[(samples_.size() / 100.) * 99]);
      }
      if (samples_.size() >= 10000) {
         fprintf(stderr, "   %-10s %10lu\n", "99.99th:", samples_[(samples_.size() / 10000.) * 9999]);
      }
   }

private:
   std::string title_;
   std::vector<uint64_t> samples_;
};

#endif
