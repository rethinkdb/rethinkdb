//  Copyright John Maddock 2009.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "required_defines.hpp"

#include "performance_measure.hpp"

#include <boost/math/special_functions/log1p.hpp>
#include <boost/math/special_functions/expm1.hpp>
#include <boost/array.hpp>

#define T double
#  include "../test/log1p_expm1_data.ipp"

template <std::size_t N>
double log1p_evaluate2(const boost::array<boost::array<T, 3>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
      result += boost::math::log1p(data[i][0]);
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(log1p_test, "log1p")
{
   double result = log1p_evaluate2(log1p_expm1_data);

   consume_result(result);
   set_call_count(
      (sizeof(log1p_expm1_data)) / sizeof(log1p_expm1_data[0]));
}

template <std::size_t N>
double expm1_evaluate2(const boost::array<boost::array<T, 3>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
      result += boost::math::expm1(data[i][0]);
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(expm1_test, "expm1")
{
   double result = expm1_evaluate2(log1p_expm1_data);

   consume_result(result);
   set_call_count(
      (sizeof(log1p_expm1_data)) / sizeof(log1p_expm1_data[0]));
}

#ifdef TEST_DCDFLIB
#include <dcdflib.h>

template <std::size_t N>
double log1p_evaluate2_dcd(const boost::array<boost::array<T, 3>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
   {
      double t = data[i][0];
      result += ::alnrel(&t);
   }
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(log1p_test_dcd, "log1p-dcd")
{
   double result = log1p_evaluate2_dcd(log1p_expm1_data);

   consume_result(result);
   set_call_count(
      (sizeof(log1p_expm1_data)) / sizeof(log1p_expm1_data[0]));
}

template <std::size_t N>
double expm1_evaluate2_dcd(const boost::array<boost::array<T, 3>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
   {
      double t = data[i][0];
      result += ::dexpm1(&t);
   }
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(expm1_test_dcd, "expm1-dcd")
{
   double result = expm1_evaluate2_dcd(log1p_expm1_data);

   consume_result(result);
   set_call_count(
      (sizeof(log1p_expm1_data)) / sizeof(log1p_expm1_data[0]));
}

#endif

#ifdef TEST_GSL

#include <gsl/gsl_sf.h>

template <std::size_t N>
double log1p_evaluate_gsl(const boost::array<boost::array<T, 3>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
      result += gsl_sf_log_1plusx(data[i][0]);
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(log1p_test, "log1p-gsl")
{
   double result = log1p_evaluate_gsl(log1p_expm1_data);

   consume_result(result);
   set_call_count(
      (sizeof(log1p_expm1_data)) / sizeof(log1p_expm1_data[0]));
}

template <std::size_t N>
double expm1_evaluate_gsl(const boost::array<boost::array<T, 3>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
      result += gsl_sf_expm1(data[i][0]);
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(expm1_test, "expm1-gsl")
{
   double result = expm1_evaluate_gsl(log1p_expm1_data);

   consume_result(result);
   set_call_count(
      (sizeof(log1p_expm1_data)) / sizeof(log1p_expm1_data[0]));
}

#endif

#ifdef TEST_CEPHES

extern "C" double expm1(double);
extern "C" double log1p(double);

template <std::size_t N>
double log1p_evaluate_cephes(const boost::array<boost::array<T, 3>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
      result += ::log1p(data[i][0]);
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(log1p_test, "log1p-cephes")
{
   double result = log1p_evaluate_cephes(log1p_expm1_data);

   consume_result(result);
   set_call_count(
      (sizeof(log1p_expm1_data)) / sizeof(log1p_expm1_data[0]));
}

template <std::size_t N>
double expm1_evaluate_cephes(const boost::array<boost::array<T, 3>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
      result += ::expm1(data[i][0]);
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(expm1_test, "expm1-cephes")
{
   double result = expm1_evaluate_cephes(log1p_expm1_data);

   consume_result(result);
   set_call_count(
      (sizeof(log1p_expm1_data)) / sizeof(log1p_expm1_data[0]));
}

#endif

