//  Copyright John Maddock 2007.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_PERFORMANCE_MEASURE_HPP
#define BOOST_MATH_PERFORMANCE_MEASURE_HPP

#include <boost/config.hpp>
#include <boost/timer.hpp>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#endif

template <class F>
double performance_measure(F f)
{
   unsigned count = 1;
   double time, result;
#ifdef _WIN32
   int old_priority = GetThreadPriority(GetCurrentThread());
   SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
#endif
   //
   // Begin by figuring out how many times to repeat
   // the function call in order to get a measureable time:
   //
   do
   {
      boost::timer t;
      for(unsigned i = 0; i < count; ++i)
         f();
      time = t.elapsed();
      count *= 2;
      t.restart();
   }while(time < 0.5);

   count /= 2;
   result = time;
   //
   // Now repeat the measurement over and over
   // and take the shortest measured time as the
   // result, generally speaking this gives us
   // consistent results:
   //
   for(unsigned i = 0; i < 20u;++i)
   {
      boost::timer t;
      for(unsigned i = 0; i < count; ++i)
         f();
      time = t.elapsed();
      if(time < result)
         result = time;
      t.restart();
   }
#ifdef _WIN32
   SetThreadPriority(GetCurrentThread(), old_priority);
#endif
   return result / count;
}

struct test_info
{
   test_info(const char* n, void (*p)());
   const char* name;
   void (*proc)();
};

inline bool operator < (const test_info& a, const test_info& b)
{
   return std::strcmp(a.name, b.name) < 0;
}

extern void consume_result(double);
extern void set_call_count(int i);
extern void add_new_test(test_info i);

inline test_info::test_info(const char* n, void (*p)())
{
   name = n;
   proc = p;
   add_new_test(*this);
}

#define BOOST_MATH_PERFORMANCE_TEST(name, string) \
   void BOOST_JOIN(name, __LINE__) ();\
   namespace{\
   test_info BOOST_JOIN(BOOST_JOIN(name, _init), __LINE__)(string, BOOST_JOIN(name, __LINE__));\
   }\
   void BOOST_JOIN(name, __LINE__) ()



#endif // BOOST_MATH_PERFORMANCE_MEASURE_HPP

