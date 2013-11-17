C++ low-latency Order Book
Joshua McKenzie
joshua.mckenzie@gmail.com

This was written as an exercise to test out a variety of techniques in
an ultra-low-latency environment.  I limited myself to 5 days from
clean slate to full implementation, so some concessions were made on
the implementation - exception handling being the primary one.

Goals:
   Write human readable view of in-memory book every 10 messages
   Write machine-readable series of mid-quotes
   On each trade message, write total qty traded at most recent trade price
   Gracefully handle various garbage messages

How to generate sample data and get performance data out of the software:
   OrderGenerator.pl should generate a stream of orders that you can
      use as input.
   An easy reference for looking at performance metrics:
      ./run_test.sh
   Or alternatively:
      ./OrderGenerator.pl > Orders.txt
      make profile
      ./OrderBookProcessor Orders.txt > test.out 2>&1
      less -n test.out

Development and testing environment:
   2.7GHz, quad-core i7 920, HT enabled
   Ubuntu 13.10
   3.8.0-19-generic #30-Ubuntu SMP
   gcc version 4.7.3 (Ubuntu/Linaro 4.7.3-1ubuntu1)
   Ubuntu clang version 3.2-1~exp9ubuntu1
   boost 1.54.0
   perl v5.14.2

Data Structure choices and Performance Aspects:
   Meta data structure:
      std::map of doubly-linked lists of orders, one per side
      std::unordered_map of all orders to get memory address of
         underlying order object
   Due to needint to keep sorted order on the book, I went with a
      std::map, rbtree implementation for the following performance
      profiles:
         Insert: logN price + hash order id
         Modify: logN to 2logN price + hash order id, depending on if
            price changed
         Delete: logN + hash
         Trade: 2logN price + variable hashing order id, based on # of
            orders blown through
      The data scope of the logN operation is limited to the price
         levels rather than being wide-open to all order_id's which
         should limit the scope of the logN cost we have to pay on
         lookup.
   Originally I considered a vector with integer-based indexing on
      price with pre-reserved memory, however calculating midpoint on
      each update and checking for a crossed book would represent a
      worst-case O(n) on *every single order*, which was clearly
      unacceptable.  Changing from the vector to the map added about 40%
      latency or so to orderAdd and the other types which I wasn't too
      thrilled about, but it's necessary.  Better logN on insert and
      staying in sorted order than paying NlogN on every insert just to
      get O(1) on trade message, though the thought crossed my mind as
      your knowledge about market conditions is often-times based on
      getting a fill before the market- data update hits.
   The map is of ulonglong price values.  On inbound messages, I parse
      and *= 100 the value to avoid double and/or float comparisons in
      the order book and get single integer comparison on the map.  While
      this only supports prices up to 92.2 quadrillion, I chose to go
      that route to bypass whole part / fract part Fixed class creation
      and comparison, as well as to support the initial design of integer
      based vector offset lookup mentioned above.  This class design
      would need to be augmented to support a Fixed class with a functor
      comparison object to allow the std::map to sort and compare via
      that key to have full double price support at the top of the price
      range.  It would be as easy as plugging the Fixed class into the
      OrderBookMap and templatizing the price value throughout the
      OrderBook in order to change over to that class.  In a production
      environment, I'd ask my client if it was acceptable for them to max
      at 92.2 quadrillion in price, and if so, go on with things.
   The map has doubly-linked lists of pointers to orders hanging off
      them.  Rather than paying a O(n) worst-case to traverse that and
      finding an order, I created a hash for the orders that should
      average to O(1).  I didn't do analysis on the default hash
      bucketing that took place, but integer hashing on order_id with a
      monotonically increasing list of #'s should be pretty evenly
      distributed.  I didn't use a standard container here as I wanted to
      be able to hash directly to a memory address and ask the container
      to delete by that address but still retain time priority.
   I'd be interested to see how shared_ptr's behaved from boost
      instead of raw pointers - as it is now the pointer maintenance
      inside the OrderBook represents both a high risk and a point of
      high mental tax on future work on the code-base.
   I went with boost::spirit::qi parsing as it's demonstrably faster
      than atoi and atof in all the benchmarks I was pulling up (up to 5x
      faster).
   strtok still reigns supreme, though it means just about everything
      in there is non-const thanks to underlying buffer modification.
   Logging is done in a separate thread, since fprintf to stderr was
      taking a pretty painful amount of time on just printing the book.
      Printing a midquote every order means we need fast IO every
      order... downside?  Crashing means lost logging state.  So don't
      crash.  ;) I didn't use exception handling in this exercise due to
      time constraints on implementation.
   The Logger burns a core.  Given the various calls median < 1 usec,
      any amount of sleeping at usec resolution could get our logging
      very very backed up.
   As to why I burned a core instead of using a conditional variable -
      those things can take a long time to get scheduled by the kernel -
      up to 10 usec or so from what I can recall off the top of my head.
      Certainly an order of magnitude longer than the time horizons I'm
      looking at working with.
   The book printing process is pretty mangled at this point,
      sprintf'ing into temporary buffers and then finally logging it.
      fprintf(stderr ...) on every message / sub-message for book-level
      printing was taking 1+ ms every 10 orders.  I managed to get it
      down to ~ 150 usec per print, depending on how deep the book is.
      That could be further optimized (char* on heap, logger deletes,
      variadic template arguments into logger instead w/different
      behavior per data type, etc) but for now, I left it as is as I ran
      out of time.  It DOES make the printBook function very ugly
      however.
   ** 2 points of interest:
      1) The OrderGenerator.pl should be parameterized.  Right now
         it's hard-coded and can produce some very odd books
      2) The program doesn't run with CLANG as there's apparently a
         known bug with 11x threads and clang that causes an exception on
         thread instantiation with binding a function.  Compile w/clang
         for clean errors, switch to gcc for release.
      2.A) UPDATE: Apparently they've fixed it since I wrote this.
         Will be interesting to do some benchmarking w/CLANG once I'm not
         on a VM and can do some bare-metal testing
      3) RDTSC on an Oracle VM will give you incorrect results as they
         virtualize calls to the TSC.  3+ usec on most operations in this
         book instead of the 300-600 nanosecond on bare metal.

Performance statistics:
   MidQuote print went from 3100 nanoseconds down to 679 median by
      implementing threaded logger_.  Note: io on critical path is bad
      (except network / necessity).
   Book print is still very very heavy on perf... sitting at 150-200
      usec or so, once per 10 orders when the book is at its heaviest.
      Not too happy about that, but optimized down from 1.3ms on printf
      based solution...

   The rollowing metrics are in nanoseconds
   addOrder:
      samples: 137872
      median:  484
      95th:    658
      99th:    1005
      99.99th: 7106
   modifyOrder:
      samples: 33944
      median:  539
      95th:    773
      99th:    1014
      99.99th: 6339
   removeOrder:
      samples: 31645
      median:  547
      95th:    705
      99th:    886
      99.99th: 7279
   trade:
      samples: 105995
      median:  1252
      95th:    1650
      99th:    2238
      99.99th: 10186

Makefile:
   make release - build the release without performance metric
      gathering or debug symbols
   make profile - build the release with rdtsc performance metric
      gathering but no debug symbols
   make debug - build with asserts available (though not widely used),
      debug printing that was compiled out, book updates on every valid
      message
