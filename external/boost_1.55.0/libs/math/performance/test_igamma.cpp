//  Copyright John Maddock 2007.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "required_defines.hpp"

#include "performance_measure.hpp"

#include <boost/math/special_functions/gamma.hpp>
#include <boost/array.hpp>

#define T double
#include "../test/igamma_big_data.ipp"
#include "../test/igamma_int_data.ipp"
#include "../test/igamma_med_data.ipp"
#include "../test/igamma_small_data.ipp"

template <std::size_t N>
double igamma_evaluate2(const boost::array<boost::array<T, 6>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
   {
      result += boost::math::gamma_p(data[i][0], data[i][1]);
      result += boost::math::gamma_q(data[i][0], data[i][1]);
   }
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(igamma_test, "igamma")
{
   double result = igamma_evaluate2(igamma_big_data);
   result += igamma_evaluate2(igamma_int_data);
   result += igamma_evaluate2(igamma_med_data);
   result += igamma_evaluate2(igamma_small_data);

   consume_result(result);
   set_call_count(
      2 * (sizeof(igamma_big_data) 
      + sizeof(igamma_int_data) 
      + sizeof(igamma_med_data)
      + sizeof(igamma_small_data)) / sizeof(igamma_big_data[0]));
}

template <std::size_t N>
double igamma_inv_evaluate2(const boost::array<boost::array<T, 6>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
   {
      result += boost::math::gamma_p_inv(data[i][0], data[i][5]);
      result += boost::math::gamma_q_inv(data[i][0], data[i][3]);
   }
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(igamma_inv_test, "igamma_inv")
{
   double result = igamma_inv_evaluate2(igamma_big_data);
   result += igamma_inv_evaluate2(igamma_int_data);
   result += igamma_inv_evaluate2(igamma_med_data);
   result += igamma_inv_evaluate2(igamma_small_data);

   consume_result(result);
   set_call_count(
      2 * (sizeof(igamma_big_data) 
      + sizeof(igamma_int_data) 
      + sizeof(igamma_med_data)
      + sizeof(igamma_small_data)) / sizeof(igamma_big_data[0]));
}

template <std::size_t N>
double igamma_inva_evaluate2(const boost::array<boost::array<T, 6>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
   {
      result += boost::math::gamma_p_inva(data[i][1], data[i][5]);
      result += boost::math::gamma_q_inva(data[i][1], data[i][3]);
   }
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(igamma_inva_test, "igamma_inva")
{
   double result = igamma_inva_evaluate2(igamma_big_data);
   result += igamma_inva_evaluate2(igamma_int_data);
   result += igamma_inva_evaluate2(igamma_med_data);
   result += igamma_inva_evaluate2(igamma_small_data);

   consume_result(result);
   set_call_count(
      2 * (sizeof(igamma_big_data) 
      + sizeof(igamma_int_data) 
      + sizeof(igamma_med_data)
      + sizeof(igamma_small_data)) / sizeof(igamma_big_data[0]));
}

#ifdef TEST_CEPHES

extern "C" {

double igam(double, double);
double igami(double, double);

}

template <std::size_t N>
double igamma_evaluate_cephes(const boost::array<boost::array<T, 6>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
      result += igam(data[i][0], data[i][1]);
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(igamma_test, "igamma-cephes")
{
   double result = igamma_evaluate_cephes(igamma_big_data);
   result += igamma_evaluate_cephes(igamma_int_data);
   result += igamma_evaluate_cephes(igamma_med_data);
   result += igamma_evaluate_cephes(igamma_small_data);

   consume_result(result);
   set_call_count(
      (sizeof(igamma_big_data) 
      + sizeof(igamma_int_data) 
      + sizeof(igamma_med_data)
      + sizeof(igamma_small_data)) / sizeof(igamma_big_data[0]));
}

template <std::size_t N>
double igamma_inv_evaluate_cephes(const boost::array<boost::array<T, 6>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
      result += igami(data[i][0], data[i][3]); // note needs complement of probability!!
   return result;
}

//
// This test does not run to completion, gets stuck
// in infinite loop inside cephes....
//
#if 0
BOOST_MATH_PERFORMANCE_TEST(igamma_inv_test, "igamma_inv-cephes")
{
   double result = igamma_inv_evaluate_cephes(igamma_big_data);
   result += igamma_inv_evaluate_cephes(igamma_int_data);
   result += igamma_inv_evaluate_cephes(igamma_med_data);
   result += igamma_inv_evaluate_cephes(igamma_small_data);

   consume_result(result);
   set_call_count(
      (sizeof(igamma_big_data) 
      + sizeof(igamma_int_data) 
      + sizeof(igamma_med_data)
      + sizeof(igamma_small_data)) / sizeof(igamma_big_data[0]));
}
#endif

#endif

#ifdef TEST_GSL

#include <gsl/gsl_sf_gamma.h>

template <std::size_t N>
double igamma_evaluate_gsl(const boost::array<boost::array<T, 6>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
      result += gsl_sf_gamma_inc_P(data[i][0], data[i][1]);
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(igamma_test, "igamma-gsl")
{
   double result = igamma_evaluate_gsl(igamma_big_data);
   result += igamma_evaluate_gsl(igamma_int_data);
   result += igamma_evaluate_gsl(igamma_med_data);
   result += igamma_evaluate_gsl(igamma_small_data);

   consume_result(result);
   set_call_count(
      (sizeof(igamma_big_data) 
      + sizeof(igamma_int_data) 
      + sizeof(igamma_med_data)
      + sizeof(igamma_small_data)) / sizeof(igamma_big_data[0]));
}

#endif

#ifdef TEST_DCDFLIB
#include <dcdflib.h>
namespace dcd{

inline double gamma_q(double x, double y)
{ 
   double ans, qans;
   int i = 0;
   gamma_inc (&x, &y, &ans, &qans, &i); 
   return qans;
}

inline double gamma_p(double x, double y)
{ 
   double ans, qans;
   int i = 0;
   gamma_inc (&x, &y, &ans, &qans, &i); 
   return ans;
}

inline double gamma_q_inv(double x, double y)
{ 
   double ans, p, nul;
   int i = 0;
   p = 1 - y;
   nul = 0;
   gamma_inc_inv (&x, &ans, &nul, &p, &y, &i); 
   return ans;
}

inline double gamma_p_inv(double x, double y)
{ 
   double ans, p, nul;
   int i = 0;
   p = 1 - y;
   nul = 0;
   gamma_inc_inv (&x, &ans, &nul, &y, &p, &i); 
   return ans;
}

}

template <std::size_t N>
double igamma_evaluate2_dcd(const boost::array<boost::array<T, 6>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
   {
      result += dcd::gamma_p(data[i][0], data[i][1]);
      result += dcd::gamma_q(data[i][0], data[i][1]);
   }
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(igamma_test, "igamma-dcd")
{
   double result = igamma_evaluate2_dcd(igamma_big_data);
   result += igamma_evaluate2_dcd(igamma_int_data);
   result += igamma_evaluate2_dcd(igamma_med_data);
   result += igamma_evaluate2_dcd(igamma_small_data);

   consume_result(result);
   set_call_count(
      2 * (sizeof(igamma_big_data) 
      + sizeof(igamma_int_data) 
      + sizeof(igamma_med_data)
      + sizeof(igamma_small_data)) / sizeof(igamma_big_data[0]));
}

template <std::size_t N>
double igamma_inv_evaluate2_dcd(const boost::array<boost::array<T, 6>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
   {
      result += dcd::gamma_p_inv(data[i][0], data[i][5]);
      result += dcd::gamma_q_inv(data[i][0], data[i][3]);
   }
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(igamma_inv_test, "igamma_inv-dcd")
{
   double result = igamma_inv_evaluate2_dcd(igamma_big_data);
   result += igamma_inv_evaluate2_dcd(igamma_int_data);
   result += igamma_inv_evaluate2_dcd(igamma_med_data);
   result += igamma_inv_evaluate2_dcd(igamma_small_data);

   consume_result(result);
   set_call_count(
      2 * (sizeof(igamma_big_data) 
      + sizeof(igamma_int_data) 
      + sizeof(igamma_med_data)
      + sizeof(igamma_small_data)) / sizeof(igamma_big_data[0]));
}

#endif

