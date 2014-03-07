// Copyright David Abrahams, Matthias Troyer, Michael Gauckler
// 2005. Distributed under the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(LIVE_CODE_TYPE)
# define LIVE_CODE_TYPE int
#endif

#include <boost/timer.hpp>

namespace test
{
  // This value is required to ensure that a smart compiler's dead
  // code elimination doesn't optimize away anything we're testing.
  // We'll use it to compute the return code of the executable to make
  // sure it's needed.
  LIVE_CODE_TYPE live_code;

  // Call objects of the given Accumulator type repeatedly with x as
  // an argument.
  template <class Accumulator, class Arg>
  void hammer(Arg const& x, long const repeats)
  {
      // Strategy: because the sum in an accumulator after each call
      // depends on the previous value of the sum, the CPU's pipeline
      // might be stalled while waiting for the previous addition to
      // complete.  Therefore, we allocate an array of accumulators,
      // and update them in sequence, so that there's no dependency
      // between adjacent addition operations.
      //
      // Additionally, if there were only one accumulator, the
      // compiler or CPU might decide to update the value in a
      // register rather that writing it back to memory.  we want each
      // operation to at least update the L1 cache.  *** Note: This
      // concern is specific to the particular application at which
      // we're targeting the test. ***

      // This has to be at least as large as the number of
      // simultaneous accumulations that can be executing in the
      // compiler pipeline.  A safe number here is larger than the
      // machine's maximum pipeline depth. If you want to test the L2
      // or L3 cache, or main memory, you can increase the size of
      // this array.  1024 is an upper limit on the pipeline depth of
      // current vector machines.
      const std::size_t number_of_accumulators = 1024;
      live_code = 0; // reset to zero

      Accumulator a[number_of_accumulators];
      
      for (long iteration = 0; iteration < repeats; ++iteration)
      {
          for (Accumulator* ap = a;  ap < a + number_of_accumulators; ++ap)
          {
              (*ap)(x);
          }
      }

      // Accumulate all the partial sums to avoid dead code
      // elimination.
      for (Accumulator* ap = a;  ap < a + number_of_accumulators; ++ap)
      {
          live_code += ap->sum;
      }
  }

  // Measure the time required to hammer accumulators of the given
  // type with the argument x.
  template <class Accumulator, class T>
  double measure(T const& x, long const repeats)
  {
      // Hammer accumulators a couple of times to ensure the
      // instruction cache is full of our test code, and that we don't
      // measure the cost of a page fault for accessing the data page
      // containing the memory where the accumulators will be
      // allocated
      hammer<Accumulator>(x, repeats);
      hammer<Accumulator>(x, repeats);

      // Now start a timer
      boost::timer time;
      hammer<Accumulator>(x, repeats);  // This time, we'll measure
      return time.elapsed() / repeats;  // return the time of one iteration
  }
}
