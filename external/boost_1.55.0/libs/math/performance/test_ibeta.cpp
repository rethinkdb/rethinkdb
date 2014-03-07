//  Copyright John Maddock 2007.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "required_defines.hpp"

#include "performance_measure.hpp"

#include <boost/math/special_functions/beta.hpp>
#include <boost/array.hpp>

#define T double
#include "../test/ibeta_data.ipp"
#include "../test/ibeta_int_data.ipp"
#include "../test/ibeta_large_data.ipp"
#include "../test/ibeta_small_data.ipp"

template <std::size_t N>
double ibeta_evaluate2(const boost::array<boost::array<T, 7>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
      result += boost::math::ibeta(data[i][0], data[i][1], data[i][2]);
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(ibeta_test, "ibeta")
{
   double result = ibeta_evaluate2(ibeta_data);
   result += ibeta_evaluate2(ibeta_int_data);
   result += ibeta_evaluate2(ibeta_large_data);
   result += ibeta_evaluate2(ibeta_small_data);

   consume_result(result);
   set_call_count(
      (sizeof(ibeta_data) 
      + sizeof(ibeta_int_data) 
      + sizeof(ibeta_large_data)
      + sizeof(ibeta_small_data)) / sizeof(ibeta_data[0]));
}

template <std::size_t N>
double ibeta_inv_evaluate2(const boost::array<boost::array<T, 7>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
      result += boost::math::ibeta_inv(data[i][0], data[i][1], data[i][5]);
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(ibeta_inv_test, "ibeta_inv")
{
   double result = ibeta_inv_evaluate2(ibeta_data);
   result += ibeta_inv_evaluate2(ibeta_int_data);
   result += ibeta_inv_evaluate2(ibeta_large_data);
   result += ibeta_inv_evaluate2(ibeta_small_data);

   consume_result(result);
   set_call_count(
      (sizeof(ibeta_data) 
      + sizeof(ibeta_int_data) 
      + sizeof(ibeta_large_data)
      + sizeof(ibeta_small_data)) / sizeof(ibeta_data[0]));
}

template <std::size_t N>
double ibeta_invab_evaluate2(const boost::array<boost::array<T, 7>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
   {
      //std::cout << "ibeta_inva(" << data[i][1] << "," << data[i][2] << "," << data[i][5] << ");" << std::endl;
      result += boost::math::ibeta_inva(data[i][1], data[i][2], data[i][5]);
      //std::cout << "ibeta_invb(" << data[i][0] << "," << data[i][2] << "," << data[i][5] << ");" << std::endl;
      result += boost::math::ibeta_invb(data[i][0], data[i][2], data[i][5]);
      //std::cout << "ibetac_inva(" << data[i][1] << "," << data[i][2] << "," << data[i][6] << ");" << std::endl;
      result += boost::math::ibetac_inva(data[i][1], data[i][2], data[i][6]);
      //std::cout << "ibetac_invb(" << data[i][0] << "," << data[i][2] << "," << data[i][6] << ");" << std::endl;
      result += boost::math::ibetac_invb(data[i][0], data[i][2], data[i][6]);
   }
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(ibeta_test, "ibeta_invab")
{
   double result = ibeta_invab_evaluate2(ibeta_data);
   result += ibeta_invab_evaluate2(ibeta_int_data);
   result += ibeta_invab_evaluate2(ibeta_large_data);
   result += ibeta_invab_evaluate2(ibeta_small_data);

   consume_result(result);
   set_call_count(
      4 * (sizeof(ibeta_data) 
      + sizeof(ibeta_int_data) 
      + sizeof(ibeta_large_data)
      + sizeof(ibeta_small_data)) / sizeof(ibeta_data[0]));
}

#ifdef TEST_CEPHES

extern "C" {

double incbet(double a, double b, double x);
double incbi(double a, double b, double y);

}

template <std::size_t N>
double ibeta_evaluate_cephes(const boost::array<boost::array<T, 7>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
      result += incbet(data[i][0], data[i][1], data[i][2]);
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(ibeta_test, "ibeta-cephes")
{
   double result = ibeta_evaluate_cephes(ibeta_data);
   result += ibeta_evaluate_cephes(ibeta_int_data);
   result += ibeta_evaluate_cephes(ibeta_large_data);
   result += ibeta_evaluate_cephes(ibeta_small_data);

   consume_result(result);
   set_call_count(
      (sizeof(ibeta_data) 
      + sizeof(ibeta_int_data) 
      + sizeof(ibeta_large_data)
      + sizeof(ibeta_small_data)) / sizeof(ibeta_data[0]));
}

template <std::size_t N>
double ibeta_inv_evaluate_cephes(const boost::array<boost::array<T, 7>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
      result += incbi(data[i][0], data[i][1], data[i][5]);
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(ibeta_inv_test, "ibeta_inv-cephes")
{
   double result = ibeta_inv_evaluate_cephes(ibeta_data);
   result += ibeta_inv_evaluate_cephes(ibeta_int_data);
   result += ibeta_inv_evaluate_cephes(ibeta_large_data);
   result += ibeta_inv_evaluate_cephes(ibeta_small_data);

   consume_result(result);
   set_call_count(
      (sizeof(ibeta_data) 
      + sizeof(ibeta_int_data) 
      + sizeof(ibeta_large_data)
      + sizeof(ibeta_small_data)) / sizeof(ibeta_data[0]));
}

#endif

#ifdef TEST_GSL
//
// This test segfaults inside GSL....
//

#include <gsl/gsl_sf.h>

template <std::size_t N>
double ibeta_evaluate_gsl(const boost::array<boost::array<T, 7>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
      result += gsl_sf_beta_inc(data[i][0], data[i][1], data[i][2]);
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(ibeta_test, "ibeta-gsl")
{
   double result = ibeta_evaluate_gsl(ibeta_data);
   result += ibeta_evaluate_gsl(ibeta_int_data);
   result += ibeta_evaluate_gsl(ibeta_large_data);
   result += ibeta_evaluate_gsl(ibeta_small_data);

   consume_result(result);
   set_call_count(
      (sizeof(ibeta_data) 
      + sizeof(ibeta_int_data) 
      + sizeof(ibeta_large_data)
      + sizeof(ibeta_small_data)) / sizeof(ibeta_data[0]));
}

#endif

#ifdef TEST_DCDFLIB
#include <dcdflib.h>

template <std::size_t N>
double ibeta_evaluate2_dcd(const boost::array<boost::array<T, 7>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
   {
      double a = data[i][0];
      double b = data[i][1];
      double x = data[i][2];
      double y = 1 - x;
      double w, w1;
      int ierr;
      beta_inc (&a, &b, &x, &y, &w, &w1, &ierr);
      result += w;
   }
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(ibeta_test_dcd, "ibeta-dcd")
{
   double result = ibeta_evaluate2_dcd(ibeta_data);
   result += ibeta_evaluate2_dcd(ibeta_int_data);
   result += ibeta_evaluate2_dcd(ibeta_large_data);
   result += ibeta_evaluate2_dcd(ibeta_small_data);

   consume_result(result);
   set_call_count(
      (sizeof(ibeta_data) 
      + sizeof(ibeta_int_data) 
      + sizeof(ibeta_large_data)
      + sizeof(ibeta_small_data)) / sizeof(ibeta_data[0]));
}

#endif
