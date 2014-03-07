//  (C) Copyright John Maddock 2008.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <pch.hpp>

#include <boost/math/concepts/real_concept.hpp>
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/math/special_functions/next.hpp>

#ifdef BOOST_MSVC
#pragma warning(disable:4127)
#endif

#if !defined(_CRAYC) && !defined(__CUDACC__) && (!defined(__GNUC__) || (__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ > 3)))
#if (defined(_M_IX86_FP) && (_M_IX86_FP >= 2)) || defined(__SSE2__) || defined(TEST_SSE2)
#include <float.h>
#include "xmmintrin.h"
#define TEST_SSE2
#endif
#endif


template <class T>
void test_value(const T& val, const char* name)
{
   using namespace boost::math;
   T upper = tools::max_value<T>();
   T lower = -upper;

   std::cout << "Testing type " << name << " with initial value " << val << std::endl;

   BOOST_CHECK_EQUAL(float_distance(float_next(val), val), -1);
   BOOST_CHECK(float_next(val) > val);
   BOOST_CHECK_EQUAL(float_distance(float_prior(val), val), 1);
   BOOST_CHECK(float_prior(val) < val);
   BOOST_CHECK_EQUAL(float_distance((boost::math::nextafter)(val, upper), val), -1);
   BOOST_CHECK((boost::math::nextafter)(val, upper) > val);
   BOOST_CHECK_EQUAL(float_distance((boost::math::nextafter)(val, lower), val), 1);
   BOOST_CHECK((boost::math::nextafter)(val, lower) < val);
   BOOST_CHECK_EQUAL(float_distance(float_next(float_next(val)), val), -2);
   BOOST_CHECK_EQUAL(float_distance(float_prior(float_prior(val)), val), 2);
   BOOST_CHECK_EQUAL(float_distance(float_prior(float_prior(val)), float_next(float_next(val))), 4);
   BOOST_CHECK_EQUAL(float_distance(float_prior(float_next(val)), val), 0);
   BOOST_CHECK_EQUAL(float_distance(float_next(float_prior(val)), val), 0);
   BOOST_CHECK_EQUAL(float_prior(float_next(val)), val);
   BOOST_CHECK_EQUAL(float_next(float_prior(val)), val);

   BOOST_CHECK_EQUAL(float_distance(float_advance(val, 4), val), -4);
   BOOST_CHECK_EQUAL(float_distance(float_advance(val, -4), val), 4);
   if(std::numeric_limits<T>::is_specialized && (std::numeric_limits<T>::has_denorm == std::denorm_present))
   {
      BOOST_CHECK_EQUAL(float_distance(float_advance(float_next(float_next(val)), 4), float_next(float_next(val))), -4);
      BOOST_CHECK_EQUAL(float_distance(float_advance(float_next(float_next(val)), -4), float_next(float_next(val))), 4);
   }
}

template <class T>
void test_values(const T& val, const char* name)
{
   static const T a = static_cast<T>(1.3456724e22);
   static const T b = static_cast<T>(1.3456724e-22);
   static const T z = 0;
   static const T one = 1;
   static const T two = 2;

   std::cout << "Testing type " << name << std::endl;

   T den = (std::numeric_limits<T>::min)() / 4;
   if(den != 0)
   {
      std::cout << "Denormals are active\n";
   }
   else
   {
      std::cout << "Denormals are flushed to zero.\n";
   }

   test_value(a, name);
   test_value(-a, name);
   test_value(b, name);
   test_value(-b, name);
   test_value(boost::math::tools::epsilon<T>(), name);
   test_value(-boost::math::tools::epsilon<T>(), name);
   test_value(boost::math::tools::min_value<T>(), name);
   test_value(-boost::math::tools::min_value<T>(), name);
   if(std::numeric_limits<T>::is_specialized && (std::numeric_limits<T>::has_denorm == std::denorm_present))
   {
      test_value(z, name);
      test_value(-z, name);
   }
   test_value(one, name);
   test_value(-one, name);
   test_value(two, name);
   test_value(-two, name);
   if(std::numeric_limits<T>::is_specialized && (std::numeric_limits<T>::has_denorm == std::denorm_present))
   {
      test_value(std::numeric_limits<T>::denorm_min(), name);
      test_value(-std::numeric_limits<T>::denorm_min(), name);
      test_value(2 * std::numeric_limits<T>::denorm_min(), name);
      test_value(-2 * std::numeric_limits<T>::denorm_min(), name);
   }

   static const int primes[] = {
      11,     13,     17,     19,     23,     29, 
      31,     37,     41,     43,     47,     53,     59,     61,     67,     71, 
      73,     79,     83,     89,     97,    101,    103,    107,    109,    113, 
      127,    131,    137,    139,    149,    151,    157,    163,    167,    173, 
      179,    181,    191,    193,    197,    199,    211,    223,    227,    229, 
      233,    239,    241,    251,    257,    263,    269,    271,    277,    281, 
      283,    293,    307,    311,    313,    317,    331,    337,    347,    349, 
      353,    359,    367,    373,    379,    383,    389,    397,    401,    409, 
      419,    421,    431,    433,    439,    443,    449,    457,    461,    463, 
   };

   for(unsigned i = 0; i < sizeof(primes)/sizeof(primes[0]); ++i)
   {
      T v1 = val;
      T v2 = val;
      for(int j = 0; j < primes[i]; ++j)
      {
         v1 = boost::math::float_next(v1);
         v2 = boost::math::float_prior(v2);
      }
      BOOST_CHECK_EQUAL(boost::math::float_distance(v1, val), -primes[i]);
      BOOST_CHECK_EQUAL(boost::math::float_distance(v2, val), primes[i]);
      BOOST_CHECK_EQUAL(boost::math::float_advance(val, primes[i]), v1);
      BOOST_CHECK_EQUAL(boost::math::float_advance(val, -primes[i]), v2);
   }
   if(std::numeric_limits<T>::is_specialized && (std::numeric_limits<T>::has_infinity))
   {
      BOOST_CHECK_EQUAL(boost::math::float_prior(std::numeric_limits<T>::infinity()), (std::numeric_limits<T>::max)());
      BOOST_CHECK_EQUAL(boost::math::float_next(-std::numeric_limits<T>::infinity()), -(std::numeric_limits<T>::max)());
      BOOST_CHECK_THROW(boost::math::float_prior(-std::numeric_limits<T>::infinity()), std::domain_error);
      BOOST_CHECK_THROW(boost::math::float_next(std::numeric_limits<T>::infinity()), std::domain_error);
      if(boost::math::policies:: BOOST_MATH_OVERFLOW_ERROR_POLICY == boost::math::policies::throw_on_error)
      {
         BOOST_CHECK_THROW(boost::math::float_prior(-(std::numeric_limits<T>::max)()), std::overflow_error);
         BOOST_CHECK_THROW(boost::math::float_next((std::numeric_limits<T>::max)()), std::overflow_error);
      }
      else
      {
         BOOST_CHECK_EQUAL(boost::math::float_prior(-(std::numeric_limits<T>::max)()), -std::numeric_limits<T>::infinity());
         BOOST_CHECK_EQUAL(boost::math::float_next((std::numeric_limits<T>::max)()), std::numeric_limits<T>::infinity());
      }
   }
}

BOOST_AUTO_TEST_CASE( test_main )
{
   test_values(1.0f, "float");
   test_values(1.0, "double");
#ifndef BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
   test_values(1.0L, "long double");
   test_values(boost::math::concepts::real_concept(0), "real_concept");
#endif
#if defined(TEST_SSE2)

#ifdef _MSC_VER
#  pragma message("Compiling SSE2 test code")
#endif
#ifdef __GNUC__
#  pragma message "Compiling SSE2 test code"
#endif

#ifdef _WIN32
   // These tests fail pretty badly on Linux x64, especially with Intel-12.1
   _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
   std::cout << "Testing again with Flush-To-Zero set" << std::endl;
   std::cout << "SSE2 control word is: " << std::hex << _mm_getcsr() << std::endl;
   test_values(1.0f, "float");
   test_values(1.0, "double");
   _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_OFF);
#endif
   BOOST_ASSERT((_mm_getcsr() & 0x40) == 0);
   _mm_setcsr(_mm_getcsr() | 0x40);
   std::cout << "Testing again with Denormals-Are-Zero set" << std::endl;
   std::cout << "SSE2 control word is: " << std::hex << _mm_getcsr() << std::endl;
   test_values(1.0f, "float");
   test_values(1.0, "double");
#endif
   
}


