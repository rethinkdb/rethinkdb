//  Copyright John Maddock 2007.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "required_defines.hpp"
#include "performance_measure.hpp"

#include <boost/math/special_functions/gamma.hpp>
#include <boost/array.hpp>

#define T double
#include "../test/erf_data.ipp"
#include "../test/erf_large_data.ipp"
#include "../test/erf_small_data.ipp"

template <std::size_t N>
double erf_evaluate2(const boost::array<boost::array<T, 3>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
      result += boost::math::erf(data[i][0]) * boost::math::erfc(data[i][0]);
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(erf_test, "erf")
{
   double result = erf_evaluate2(erf_data);
   result += erf_evaluate2(erf_large_data);
   result += erf_evaluate2(erf_small_data);

   consume_result(result);
   set_call_count(2 * (sizeof(erf_data) + sizeof(erf_large_data) + sizeof(erf_small_data)) / sizeof(erf_data[0]));
}

template <std::size_t N>
double erf_inv_evaluate2(const boost::array<boost::array<T, 3>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
      result += boost::math::erf_inv(data[i][1]);
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(erf_inv_test, "erf_inv")
{
   double result = erf_inv_evaluate2(erf_data);
   result += erf_inv_evaluate2(erf_large_data);
   result += erf_inv_evaluate2(erf_small_data);

   consume_result(result);
   set_call_count((sizeof(erf_data) + sizeof(erf_large_data) + sizeof(erf_small_data)) / sizeof(erf_data[0]));
}

#ifdef TEST_CEPHES

extern "C" {

double erf(double);
double erfc(double);

}

template <std::size_t N>
double erf_evaluate_cephes(const boost::array<boost::array<T, 3>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
      result += erf(data[i][0]) * erfc(data[i][0]);
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(erf_test, "erf-cephes")
{
   double result = erf_evaluate_cephes(erf_data);
   result += erf_evaluate_cephes(erf_large_data);
   result += erf_evaluate_cephes(erf_small_data);

   consume_result(result);
   set_call_count(2 * (sizeof(erf_data) + sizeof(erf_large_data) + sizeof(erf_small_data)) / sizeof(erf_data[0]));
}

#endif

#ifdef TEST_GSL

#include <gsl/gsl_sf.h>

template <std::size_t N>
double erf_evaluate_gsl(const boost::array<boost::array<T, 3>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
      result += gsl_sf_erf(data[i][0]) * gsl_sf_erfc(data[i][0]);
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(erf_test, "erf-gsl")
{
   double result = erf_evaluate_gsl(erf_data);
   result += erf_evaluate_gsl(erf_large_data);
   result += erf_evaluate_gsl(erf_small_data);

   consume_result(result);
   set_call_count(2 * (sizeof(erf_data) + sizeof(erf_large_data) + sizeof(erf_small_data)) / sizeof(erf_data[0]));
}

#endif


#ifdef TEST_DCDFLIB
#include <dcdflib.h>

template <std::size_t N>
double erf_evaluate2_dcd(const boost::array<boost::array<T, 3>, N>& data)
{
   int scale = 0;
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
   {
      double x = data[i][0];
      result += error_f(&x) + error_fc(&scale, &x);
   }
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(erf_test_dcd, "erf-dcd")
{
   double result = erf_evaluate2_dcd(erf_data);
   result += erf_evaluate2_dcd(erf_large_data);
   result += erf_evaluate2_dcd(erf_small_data);

   consume_result(result);
   set_call_count(2 * (sizeof(erf_data) + sizeof(erf_large_data) + sizeof(erf_small_data)) / sizeof(erf_data[0]));
}

#endif

