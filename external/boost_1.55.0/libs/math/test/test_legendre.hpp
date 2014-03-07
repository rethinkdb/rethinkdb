// Copyright John Maddock 2006.
// Copyright Paul A. Bristow 2007, 2009
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifdef _MSC_VER
#  pragma warning (disable : 4756) // overflow in constant arithmetic
#endif

#include <boost/math/concepts/real_concept.hpp>
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/math/special_functions/math_fwd.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/array.hpp>
#include "functor.hpp"

#include "handle_test_result.hpp"
#include "test_legendre_hooks.hpp"
#include "table_type.hpp"

#ifndef SC_
#define SC_(x) static_cast<typename table_type<T>::type>(BOOST_JOIN(x, L))
#endif

template <class Real, class T>
void do_test_legendre_p(const T& data, const char* type_name, const char* test_name)
{
   typedef Real                   value_type;

   typedef value_type (*pg)(int, value_type);
#if defined(BOOST_MATH_NO_DEDUCED_FUNCTION_POINTERS)
   pg funcp = boost::math::legendre_p<value_type>;
#else
   pg funcp = boost::math::legendre_p;
#endif

   boost::math::tools::test_result<value_type> result;

   std::cout << "Testing " << test_name << " with type " << type_name
      << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";

   //
   // test legendre_p against data:
   //
   result = boost::math::tools::test_hetero<Real>(
      data,
      bind_func_int1<Real>(funcp, 0, 1),
      extract_result<Real>(2));
   handle_test_result(result, data[result.worst()], result.worst(), type_name, "boost::math::legendre_p", test_name);
#ifdef TEST_OTHER
   if(::boost::is_floating_point<value_type>::value){
      funcp = other::legendre_p;
   result = boost::math::tools::test_hetero<Real>(
      data,
      bind_func_int1<Real>(funcp, 0, 1),
      extract_result<Real>(2));
      print_test_result(result, data[result.worst()], result.worst(), type_name, "other::legendre_p");
   }
#endif

   typedef value_type (*pg2)(unsigned, value_type);
#if defined(BOOST_MATH_NO_DEDUCED_FUNCTION_POINTERS)
   pg2 funcp2 = boost::math::legendre_q<value_type>;
#else
   pg2 funcp2 = boost::math::legendre_q;
#endif

   //
   // test legendre_q against data:
   //
   result = boost::math::tools::test_hetero<Real>(
      data,
      bind_func_int1<Real>(funcp2, 0, 1),
      extract_result<Real>(3));
   handle_test_result(result, data[result.worst()], result.worst(), type_name, "boost::math::legendre_q", test_name);
#ifdef TEST_OTHER
   if(::boost::is_floating_point<value_type>::value){
      funcp = other::legendre_q;
   result = boost::math::tools::test_hetero<Real>(
      data,
      bind_func_int1<Real>(funcp2, 0, 1),
      extract_result<Real>(3));
      print_test_result(result, data[result.worst()], result.worst(), type_name, "other::legendre_q");
   }
#endif


   std::cout << std::endl;
}

template <class Real, class T>
void do_test_assoc_legendre_p(const T& data, const char* type_name, const char* test_name)
{
   typedef Real                   value_type;

   typedef value_type (*pg)(int, int, value_type);
#if defined(BOOST_MATH_NO_DEDUCED_FUNCTION_POINTERS)
   pg funcp = boost::math::legendre_p<value_type>;
#else
   pg funcp = boost::math::legendre_p;
#endif

   boost::math::tools::test_result<value_type> result;

   std::cout << "Testing " << test_name << " with type " << type_name
      << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";

   //
   // test legendre_p against data:
   //
   result = boost::math::tools::test_hetero<Real>(
      data,
      bind_func_int2<Real>(funcp, 0, 1, 2),
      extract_result<Real>(3));
   handle_test_result(result, data[result.worst()], result.worst(), type_name, "boost::math::legendre_p", test_name);
   std::cout << std::endl;
}

template <class T>
void test_legendre_p(T, const char* name)
{
   //
   // The actual test data is rather verbose, so it's in a separate file
   //
   // The contents are as follows, each row of data contains
   // three items, input value a, input value b and erf(a, b):
   //
#  include "legendre_p.ipp"

   do_test_legendre_p<T>(legendre_p, name, "Legendre Polynomials: Small Values");

#  include "legendre_p_large.ipp"

   do_test_legendre_p<T>(legendre_p_large, name, "Legendre Polynomials: Large Values");

#  include "assoc_legendre_p.ipp"

   do_test_assoc_legendre_p<T>(assoc_legendre_p, name, "Associated Legendre Polynomials: Small Values");

}

template <class T>
void test_spots(T, const char* t)
{
   std::cout << "Testing basic sanity checks for type " << t << std::endl;
   //
   // basic sanity checks, tolerance is 100 epsilon:
   //
   T tolerance = boost::math::tools::epsilon<T>() * 100;
   BOOST_CHECK_CLOSE_FRACTION(::boost::math::legendre_p(1, static_cast<T>(0.5L)), static_cast<T>(0.5L), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(::boost::math::legendre_p(-1, static_cast<T>(0.5L)), static_cast<T>(1L), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(::boost::math::legendre_p(4, static_cast<T>(0.5L)), static_cast<T>(-0.2890625000000000000000000000000000000000L), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(::boost::math::legendre_p(-4, static_cast<T>(0.5L)), static_cast<T>(-0.4375000000000000000000000000000000000000L), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(::boost::math::legendre_p(7, static_cast<T>(0.5L)), static_cast<T>(0.2231445312500000000000000000000000000000L), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(::boost::math::legendre_p(-7, static_cast<T>(0.5L)), static_cast<T>(0.3232421875000000000000000000000000000000L), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(::boost::math::legendre_p(40, static_cast<T>(0.5L)), static_cast<T>(-0.09542943523261546936538467572384923220258L), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(::boost::math::legendre_p(-40, static_cast<T>(0.5L)), static_cast<T>(-0.1316993126940266257030910566308990611306L), tolerance);

   BOOST_CHECK_CLOSE_FRACTION(::boost::math::legendre_p(4, 2, static_cast<T>(0.5L)), static_cast<T>(4.218750000000000000000000000000000000000L), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(::boost::math::legendre_p(-4, 2, static_cast<T>(0.5L)), static_cast<T>(5.625000000000000000000000000000000000000L), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(::boost::math::legendre_p(7, 5, static_cast<T>(0.5L)), static_cast<T>(-5696.789530152175143607977274672800795328L), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(::boost::math::legendre_p(-7, 4, static_cast<T>(0.5L)), static_cast<T>(465.1171875000000000000000000000000000000L), tolerance);
   if(std::numeric_limits<T>::max_exponent > std::numeric_limits<float>::max_exponent)
   {
      BOOST_CHECK_CLOSE_FRACTION(::boost::math::legendre_p(40, 30, static_cast<T>(0.5L)), static_cast<T>(-7.855722083232252643913331343916012143461e45L), tolerance);
   }
   BOOST_CHECK_CLOSE_FRACTION(::boost::math::legendre_p(-40, 20, static_cast<T>(0.5L)), static_cast<T>(4.966634149702370788037088925152355134665e30L), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(::boost::math::legendre_p(4, 2, static_cast<T>(-0.5L)), static_cast<T>(4.218750000000000000000000000000000000000L), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(::boost::math::legendre_p(-4, 2, static_cast<T>(-0.5L)), static_cast<T>(-5.625000000000000000000000000000000000000L), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(::boost::math::legendre_p(7, 5, static_cast<T>(-0.5L)), static_cast<T>(-5696.789530152175143607977274672800795328L), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(::boost::math::legendre_p(-7, 4, static_cast<T>(-0.5L)), static_cast<T>(465.1171875000000000000000000000000000000L), tolerance);
   if(std::numeric_limits<T>::max_exponent > std::numeric_limits<float>::max_exponent)
   {
      BOOST_CHECK_CLOSE_FRACTION(::boost::math::legendre_p(40, 30, static_cast<T>(-0.5L)), static_cast<T>(-7.855722083232252643913331343916012143461e45L), tolerance);
   }
   BOOST_CHECK_CLOSE_FRACTION(::boost::math::legendre_p(-40, 20, static_cast<T>(-0.5L)), static_cast<T>(-4.966634149702370788037088925152355134665e30L), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(::boost::math::legendre_p(4, -2, static_cast<T>(0.5L)), static_cast<T>(0.01171875000000000000000000000000000000000L), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(::boost::math::legendre_p(-4, -2, static_cast<T>(0.5L)), static_cast<T>(0.04687500000000000000000000000000000000000L), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(::boost::math::legendre_p(7, -5, static_cast<T>(0.5L)), static_cast<T>(0.00002378609812640364935569308025139290054701L), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(::boost::math::legendre_p(-7, -4, static_cast<T>(0.5L)), static_cast<T>(0.0002563476562500000000000000000000000000000L), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(::boost::math::legendre_p(40, -30, static_cast<T>(0.5L)), static_cast<T>(-2.379819988646847616996471299410611801239e-48L), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(::boost::math::legendre_p(-40, -20, static_cast<T>(0.5L)), static_cast<T>(4.356454600748202401657099008867502679122e-33L), tolerance);

   BOOST_CHECK_CLOSE_FRACTION(::boost::math::legendre_q(1, static_cast<T>(0.5L)), static_cast<T>(-0.7253469278329725771511886907693685738381L), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(::boost::math::legendre_q(4, static_cast<T>(0.5L)), static_cast<T>(0.4401745259867706044988642951843745400835L), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(::boost::math::legendre_q(7, static_cast<T>(0.5L)), static_cast<T>(-0.3439152932669753451878700644212067616780L), tolerance);
   BOOST_CHECK_CLOSE_FRACTION(::boost::math::legendre_q(40, static_cast<T>(0.5L)), static_cast<T>(0.1493671665503550095010454949479907886011L), tolerance);
}

