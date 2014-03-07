//  Copyright John Maddock 2007.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "required_defines.hpp"
#include "performance_measure.hpp"

#include <boost/math/special_functions/cbrt.hpp>

double cbrt_test()
{
   double result = 0;
   double val = 1e-100;
   for(int i = 0; i < 1000; ++i)
   {
      val *= 1.5;
      result += boost::math::cbrt(val);
   }
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(cbrt_test, "cbrt")
{
   double result = cbrt_test();

   consume_result(result);
   set_call_count(1000);
}

double cbrt_pow_test()
{
   double result = 0;
   double val = 1e-100;
   for(int i = 0; i < 1000; ++i)
   {
      val *= 1.5;
      result += std::pow(val, 0.33333333333333333333333333333333);
   }
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(cbrt_pow_test, "cbrt-pow")
{
   double result = cbrt_pow_test();

   consume_result(result);
   set_call_count(1000);
}

#ifdef TEST_CEPHES

extern "C" double cbrt(double);

double cbrt_cephes_test()
{
   double result = 0;
   double val = 1e-100;
   for(int i = 0; i < 1000; ++i)
   {
      val *= 1.5;
      result += ::cbrt(val);
   }
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(cbrt_test, "cbrt-cephes")
{
   double result = cbrt_cephes_test();

   consume_result(result);
   set_call_count(1000);
}

#endif

#if defined(__GNUC__) && (__GNUC__ >= 4)

#include <math.h>

double cbrt_c99_test()
{
   double result = 0;
   double val = 1e-100;
   for(int i = 0; i < 1000; ++i)
   {
      val *= 1.5;
      result += ::cbrt(val);
   }
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(cbrt_c99_test, "cbrt-c99")
{
   double result = cbrt_c99_test();

   consume_result(result);
   set_call_count(1000);
}

#endif
