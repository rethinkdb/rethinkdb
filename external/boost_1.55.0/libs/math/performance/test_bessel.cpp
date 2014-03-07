//  Copyright John Maddock 2009.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "required_defines.hpp"

#include "performance_measure.hpp"

#include <boost/math/special_functions/bessel.hpp>
#include <boost/array.hpp>

#define T double
#include "../test/bessel_j_data.ipp"
#include "../test/bessel_j_int_data.ipp"
#include "../test/bessel_j_large_data.ipp"
#include "../test/bessel_y01_data.ipp"
#include "../test/bessel_yn_data.ipp"
#include "../test/bessel_yv_data.ipp"
#include "../test/bessel_k_int_data.ipp"
#include "../test/bessel_k_data.ipp"
#include "../test/bessel_i_int_data.ipp"
#include "../test/bessel_i_data.ipp"
#include "../test/sph_bessel_data.ipp"
#include "../test/sph_neumann_data.ipp"


template <std::size_t N>
double bessel_evaluate2(const boost::array<boost::array<T, 3>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
      result += boost::math::cyl_bessel_j(data[i][0], data[i][1]);
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(bessel_test, "cyl_bessel_j")
{
   double result= bessel_evaluate2(bessel_j_data);
   result += bessel_evaluate2(bessel_j_int_data);
   result += bessel_evaluate2(bessel_j_large_data);

   consume_result(result);
   set_call_count(
      (sizeof(bessel_j_data) 
      + sizeof(bessel_j_int_data) 
      + sizeof(bessel_j_large_data)) / sizeof(bessel_j_data[0]));
}

template <std::size_t N>
double bessel_y_evaluate2(const boost::array<boost::array<T, 3>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
      result += boost::math::cyl_neumann(data[i][0], data[i][1]);
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(bessel_test, "cyl_neumann")
{
   double result= bessel_y_evaluate2(bessel_y01_data);
   result += bessel_y_evaluate2(bessel_yn_data);
   result += bessel_y_evaluate2(bessel_yv_data);

   consume_result(result);
   set_call_count(
      (sizeof(bessel_y01_data) 
      + sizeof(bessel_yn_data) 
      + sizeof(bessel_yv_data)) / sizeof(bessel_j_data[0]));
}

template <std::size_t N>
double bessel_i_evaluate(const boost::array<boost::array<T, 3>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
      result += boost::math::cyl_bessel_i(data[i][0], data[i][1]);
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(bessel_test, "cyl_bessel_i")
{
   double result= bessel_i_evaluate(bessel_i_int_data);
   result += bessel_i_evaluate(bessel_i_data);

   consume_result(result);
   set_call_count(
      (sizeof(bessel_i_data) 
      + sizeof(bessel_i_int_data)) / sizeof(bessel_i_data[0]));
}

template <std::size_t N>
double bessel_k_evaluate(const boost::array<boost::array<T, 3>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
      result += boost::math::cyl_bessel_k(data[i][0], data[i][1]);
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(bessel_test, "cyl_bessel_k")
{
   double result= bessel_k_evaluate(bessel_k_data);
   result += bessel_k_evaluate(bessel_k_int_data);

   consume_result(result);
   set_call_count(
      (sizeof(bessel_k_data) 
      + sizeof(bessel_k_int_data)) / sizeof(bessel_k_data[0]));
}

template <std::size_t N>
double bessel_sph_j_evaluate(const boost::array<boost::array<T, 3>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
      result += boost::math::sph_bessel(static_cast<unsigned>(data[i][0]), data[i][1]);
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(bessel_test, "sph_bessel")
{
   double result= bessel_sph_j_evaluate(sph_bessel_data);

   consume_result(result);
   set_call_count(
      sizeof(sph_bessel_data) / sizeof(sph_bessel_data[0]));
}

template <std::size_t N>
double bessel_sph_y_evaluate(const boost::array<boost::array<T, 3>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
      result += boost::math::sph_neumann(static_cast<unsigned>(data[i][0]), data[i][1]);
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(bessel_test, "sph_neumann")
{
   double result= bessel_sph_y_evaluate(sph_neumann_data);

   consume_result(result);
   set_call_count(
      sizeof(sph_neumann_data) / sizeof(sph_neumann_data[0]));
}

#ifdef TEST_DCDFLIB

#endif

#ifdef TEST_CEPHES

extern "C" double jv(double, double);
extern "C" double yv(double, double);
extern "C" double iv(double, double);

template <std::size_t N>
double bessel_evaluate2_cephes(const boost::array<boost::array<T, 3>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
      result += jv(data[i][0], data[i][1]);
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(bessel_test, "cyl_bessel_j-cephes")
{
   double result = bessel_evaluate2_cephes(bessel_j_data);
   result += bessel_evaluate2_cephes(bessel_j_int_data);
   result += bessel_evaluate2_cephes(bessel_j_large_data);

   consume_result(result);
   set_call_count(
      (sizeof(bessel_j_data) 
      + sizeof(bessel_j_int_data) 
      + sizeof(bessel_j_large_data)) / sizeof(bessel_j_data[0]));
}

template <std::size_t N>
double bessel_y_evaluate2_cephes(const boost::array<boost::array<T, 3>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
      result += yv(data[i][0], data[i][1]);
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(bessel_test, "cyl_neumann-cephes")
{
   double result= bessel_y_evaluate2_cephes(bessel_y01_data);
   result += bessel_y_evaluate2_cephes(bessel_yn_data);
   result += bessel_y_evaluate2_cephes(bessel_yv_data);

   consume_result(result);
   set_call_count(
      (sizeof(bessel_y01_data) 
      + sizeof(bessel_yn_data) 
      + sizeof(bessel_yv_data)) / sizeof(bessel_j_data[0]));
}

template <std::size_t N>
double bessel_i_evaluate_cephes(const boost::array<boost::array<T, 3>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
      result += iv(data[i][0], data[i][1]);
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(bessel_test, "cyl_bessel_i-cephes")
{
   double result= bessel_i_evaluate_cephes(bessel_i_int_data);
   result += bessel_i_evaluate_cephes(bessel_i_data);

   consume_result(result);
   set_call_count(
      (sizeof(bessel_i_data) 
      + sizeof(bessel_i_int_data)) / sizeof(bessel_i_data[0]));
}

#endif

#ifdef TEST_GSL

#include <gsl/gsl_sf_bessel.h>

template <std::size_t N>
double bessel_evaluate2_gsl(const boost::array<boost::array<T, 3>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
      result += gsl_sf_bessel_Jnu(data[i][0], data[i][1]);
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(bessel_test, "cyl_bessel_j-gsl")
{
   double result = bessel_evaluate2_gsl(bessel_j_data);
   result += bessel_evaluate2_gsl(bessel_j_int_data);
   result += bessel_evaluate2_gsl(bessel_j_large_data);

   consume_result(result);
   set_call_count(
      (sizeof(bessel_j_data) 
      + sizeof(bessel_j_int_data) 
      + sizeof(bessel_j_large_data)) / sizeof(bessel_j_data[0]));
}

template <std::size_t N>
double bessel_y_evaluate2_gsl(const boost::array<boost::array<T, 3>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
      result += gsl_sf_bessel_Ynu(data[i][0], data[i][1]);
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(bessel_test, "cyl_neumann-gsl")
{
   double result= bessel_y_evaluate2_gsl(bessel_y01_data);
   result += bessel_y_evaluate2_gsl(bessel_yn_data);
   result += bessel_y_evaluate2_gsl(bessel_yv_data);

   consume_result(result);
   set_call_count(
      (sizeof(bessel_y01_data) 
      + sizeof(bessel_yn_data) 
      + sizeof(bessel_yv_data)) / sizeof(bessel_j_data[0]));
}

template <std::size_t N>
double bessel_i_evaluate_gsl(const boost::array<boost::array<T, 3>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
      result += gsl_sf_bessel_Inu(data[i][0], data[i][1]);
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(bessel_test, "cyl_bessel_i-gsl")
{
   double result= bessel_i_evaluate_gsl(bessel_i_int_data);
   result += bessel_i_evaluate_gsl(bessel_i_data);

   consume_result(result);
   set_call_count(
      (sizeof(bessel_i_data) 
      + sizeof(bessel_i_int_data)) / sizeof(bessel_i_data[0]));
}

template <std::size_t N>
double bessel_k_evaluate_gsl(const boost::array<boost::array<T, 3>, N>& data)
{
   double result = 0;
   for(unsigned i = 0; i < N; ++i)
      result += gsl_sf_bessel_Knu(data[i][0], data[i][1]);
   return result;
}

BOOST_MATH_PERFORMANCE_TEST(bessel_test, "cyl_bessel_k-gsl")
{
   double result= bessel_k_evaluate_gsl(bessel_k_data);
   result += bessel_k_evaluate_gsl(bessel_k_int_data);

   consume_result(result);
   set_call_count(
      (sizeof(bessel_k_data) 
      + sizeof(bessel_k_int_data)) / sizeof(bessel_k_data[0]));
}

#endif

