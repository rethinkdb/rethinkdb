// Copyright David Abrahams, Matthias Troyer, Michael Gauckler
// 2005. Distributed under the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/parameter.hpp>
#include <boost/timer.hpp>
#include <iostream>

namespace test
{
  //
  // This test measures the abstraction overhead of using the named
  // parameter interface.  Some actual test results have been recorded
  // in timings.txt in this source file's directory, or
  // http://www.boost.org/libs/parameter/test/timings.txt.
  //
  // Caveats:
  //
  //   1. This test penalizes the named parameter library slightly, by
  //      passing two arguments through the named interface, while
  //      only passing one through the plain C++ interface.
  //
  //   2. This test does not measure the case where an ArgumentPack is
  //      so large that it doesn't fit in the L1 cache.
  //
  //   3. Although we've tried to make this test as general as
  //      possible, we are targeting it at a specific application.
  //      Where that affects design decisions, we've noted it below in
  //      ***...***.
  //
  //   4. The first time you run this program, the time may not be
  //      representative because of disk and memory cache effects, so
  //      always run it multiple times and ignore the first
  //      measurement.  This approach will also allow you to estimate
  //      the statistical error of your test by observing the
  //      variation in the valid times.
  //
  //   5. Try to run this program on a machine that's otherwise idle,
  //      or other processes and even device hardware interrupts may
  //      interfere by causing caches to be flushed.
  
  // Accumulator function object with plain C++ interface 
  template <class T>
  struct plain_weight_running_total
  {
      plain_weight_running_total()
#if BOOST_WORKAROUND(BOOST_MSVC, < 1300)
        : sum(T())
#else
        : sum()
#endif 
      {}
      
      void operator()(T w)
      {
          this->sum += w;
      }

      T sum;
  };

  BOOST_PARAMETER_KEYWORD(tag, weight)
  BOOST_PARAMETER_KEYWORD(tag, value)
      
  // Accumulator function object with named parameter interface
  template <class T>
  struct named_param_weight_running_total
  {
      named_param_weight_running_total()
#if BOOST_WORKAROUND(BOOST_MSVC, < 1300)
        : sum(T())
#else
        : sum()
#endif 
      {}

      template <class ArgumentPack>
      void operator()(ArgumentPack const& variates)
      {
          this->sum += variates[weight];
      }

      T sum;
  };

  // This value is required to ensure that a smart compiler's dead
  // code elimination doesn't optimize away anything we're testing.
  // We'll use it to compute the return code of the executable to make
  // sure it's needed.
  double live_code;

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
      return time.elapsed();
  }
}

int main()
{
    using namespace test;

    // first decide how many repetitions to measure
    long repeats = 100;
    double measured = 0;
    while (measured < 1.0 && repeats <= 10000000)
    {
        repeats *= 10;
        
        boost::timer time;

        hammer<plain_weight_running_total<double> >(.1, repeats);
        hammer<named_param_weight_running_total<double> >(
            (weight = .1, value = .2), repeats);

        measured = time.elapsed();
    }
    
    std::cout
        << "plain time:           "
        << measure<plain_weight_running_total<double> >(.1, repeats)
        << std::endl;
    
    std::cout
        << "named parameter time: "
        << measure<named_param_weight_running_total<double> >(
            (weight = .1, value = .2), repeats
        )
        << std::endl;

    // This is ultimately responsible for preventing all the test code
    // from being optimized away.  Change this to return 0 and you
    // unplug the whole test's life support system.
    return live_code < 0.;
}
