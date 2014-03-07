// Copyright John Maddock 2006.
// Copyright Paul A. Bristow 2007, 2009
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/math/concepts/real_concept.hpp>
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/math/special_functions/math_fwd.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/type_traits/is_floating_point.hpp>
#include <boost/array.hpp>
#include "functor.hpp"

#include "test_erf_hooks.hpp"
#include "handle_test_result.hpp"
#include "table_type.hpp"

#ifndef SC_
#define SC_(x) static_cast<typename table_type<T>::type>(BOOST_JOIN(x, L))
#endif

template <class Real, class T>
void do_test_erf(const T& data, const char* type_name, const char* test_name)
{
   typedef Real                   value_type;

   typedef value_type (*pg)(value_type);
#if defined(BOOST_MATH_NO_DEDUCED_FUNCTION_POINTERS)
   pg funcp = boost::math::erf<value_type>;
#else
   pg funcp = boost::math::erf;
#endif

   boost::math::tools::test_result<value_type> result;

   std::cout << "Testing " << test_name << " with type " << type_name
      << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";

   //
   // test erf against data:
   //
   result = boost::math::tools::test_hetero<Real>(
      data,
      bind_func<Real>(funcp, 0),
      extract_result<Real>(1));
   handle_test_result(result, data[result.worst()], result.worst(), type_name, "boost::math::erf", test_name);
#ifdef TEST_OTHER
   if(::boost::is_floating_point<value_type>::value){
      funcp = other::erf;
      result = boost::math::tools::test_hetero<Real>(
         data,
         bind_func<Real>(funcp, 0),
         extract_result<Real>(1));
      print_test_result(result, data[result.worst()], result.worst(), type_name, "other::erf");
   }
#endif
   //
   // test erfc against data:
   //
#if defined(BOOST_MATH_NO_DEDUCED_FUNCTION_POINTERS)
   funcp = boost::math::erfc<value_type>;
#else
   funcp = boost::math::erfc;
#endif
   result = boost::math::tools::test_hetero<Real>(
      data,
      bind_func<Real>(funcp, 0),
      extract_result<Real>(2));
   handle_test_result(result, data[result.worst()], result.worst(), type_name, "boost::math::erfc", test_name);
#ifdef TEST_OTHER
   if(::boost::is_floating_point<value_type>::value){
      funcp = other::erfc;
      result = boost::math::tools::test_hetero<Real>(
         data,
         bind_func<Real>(funcp, 0),
         extract_result<Real>(2));
      print_test_result(result, data[result.worst()], result.worst(), type_name, "other::erfc");
   }
#endif
   std::cout << std::endl;
}

template <class Real, class T>
void do_test_erf_inv(const T& data, const char* type_name, const char* test_name)
{
   typedef Real                   value_type;

   typedef value_type (*pg)(value_type);

   boost::math::tools::test_result<value_type> result;
   std::cout << "Testing " << test_name << " with type " << type_name
      << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
   //
   // test erf_inv against data:
   //
#if defined(BOOST_MATH_NO_DEDUCED_FUNCTION_POINTERS)
   pg funcp = boost::math::erf_inv<value_type>;
#else
   pg funcp = boost::math::erf_inv;
#endif
   result = boost::math::tools::test_hetero<Real>(
      data,
      bind_func<Real>(funcp, 0),
      extract_result<Real>(1));
   handle_test_result(result, data[result.worst()], result.worst(), type_name, "boost::math::erf_inv", test_name);
   std::cout << std::endl;
}

template <class Real, class T>
void do_test_erfc_inv(const T& data, const char* type_name, const char* test_name)
{
   typedef Real                   value_type;

   typedef value_type (*pg)(value_type);
#if defined(BOOST_MATH_NO_DEDUCED_FUNCTION_POINTERS)
   pg funcp = boost::math::erf<value_type>;
#else
   pg funcp = boost::math::erf;
#endif

   boost::math::tools::test_result<value_type> result;
   std::cout << "Testing " << test_name << " with type " << type_name
      << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
   //
   // test erfc_inv against data:
   //
#if defined(BOOST_MATH_NO_DEDUCED_FUNCTION_POINTERS)
   funcp = boost::math::erfc_inv<value_type>;
#else
   funcp = boost::math::erfc_inv;
#endif
   result = boost::math::tools::test_hetero<Real>(
      data,
      bind_func<Real>(funcp, 0),
      extract_result<Real>(1));
   handle_test_result(result, data[result.worst()], result.worst(), type_name, "boost::math::erfc_inv", test_name);
   std::cout << std::endl;
}

template <class T>
void test_erf(T, const char* name)
{
   //
   // The actual test data is rather verbose, so it's in a separate file
   //
   // The contents are as follows, each row of data contains
   // three items, input value a, input value b and erf(a, b):
   //
#  include "erf_small_data.ipp"

   do_test_erf<T>(erf_small_data, name, "Erf Function: Small Values");

#  include "erf_data.ipp"

   do_test_erf<T>(erf_data, name, "Erf Function: Medium Values");

#  include "erf_large_data.ipp"

   do_test_erf<T>(erf_large_data, name, "Erf Function: Large Values");

#  include "erf_inv_data.ipp"

   do_test_erf_inv<T>(erf_inv_data, name, "Inverse Erf Function");

#  include "erfc_inv_data.ipp"

   do_test_erfc_inv<T>(erfc_inv_data, name, "Inverse Erfc Function");

#  include "erfc_inv_big_data.ipp"

   if(std::numeric_limits<T>::min_exponent <= -4500)
   {
      do_test_erfc_inv<T>(erfc_inv_big_data, name, "Inverse Erfc Function: extreme values");
   }
}

template <class T>
void test_spots(T, const char* t)
{
   std::cout << "Testing basic sanity checks for type " << t << std::endl;
   //
   // basic sanity checks, tolerance is 10 epsilon expressed as a percentage:
   //
   T tolerance = boost::math::tools::epsilon<T>() * 1000;
   BOOST_CHECK_CLOSE(::boost::math::erfc(static_cast<T>(0.125)), static_cast<T>(0.85968379519866618260697055347837660181302041685015L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::erfc(static_cast<T>(0.5)), static_cast<T>(0.47950012218695346231725334610803547126354842424204L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::erfc(static_cast<T>(1)), static_cast<T>(0.15729920705028513065877936491739074070393300203370L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::erfc(static_cast<T>(5)), static_cast<T>(1.5374597944280348501883434853833788901180503147234e-12L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::erfc(static_cast<T>(-0.125)), static_cast<T>(1.1403162048013338173930294465216233981869795831498L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::erfc(static_cast<T>(-0.5)), static_cast<T>(1.5204998778130465376827466538919645287364515757580L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::erfc(static_cast<T>(0)), static_cast<T>(1), tolerance);

   BOOST_CHECK_CLOSE(::boost::math::erf(static_cast<T>(0.125)), static_cast<T>(0.14031620480133381739302944652162339818697958314985L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::erf(static_cast<T>(0.5)), static_cast<T>(0.52049987781304653768274665389196452873645157575796L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::erf(static_cast<T>(1)), static_cast<T>(0.84270079294971486934122063508260925929606699796630L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::erf(static_cast<T>(5)), static_cast<T>(0.9999999999984625402055719651498116565146166211099L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::erf(static_cast<T>(-0.125)), static_cast<T>(-0.14031620480133381739302944652162339818697958314985L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::erf(static_cast<T>(-0.5)), static_cast<T>(-0.52049987781304653768274665389196452873645157575796L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::erf(static_cast<T>(0)), static_cast<T>(0), tolerance);

   tolerance = boost::math::tools::epsilon<T>() * 100 * 200; // 200 eps %.
#if defined(__CYGWIN__)
   // some platforms long double is only reliably accurate to double precision:
   if(sizeof(T) == sizeof(long double))
      tolerance = boost::math::tools::epsilon<double>() * 100 * 200; // 200 eps %.
#endif

   for(T i = -0.95f; i < 1; i += 0.125f)
   {
      T inv = boost::math::erf_inv(i);
      T b = boost::math::erf(inv);
      BOOST_CHECK_CLOSE(b, i, tolerance);
   }
   for(T j = 0.125f; j < 2; j += 0.125f)
   {
      T inv = boost::math::erfc_inv(j);
      T b = boost::math::erfc(inv);
      BOOST_CHECK_CLOSE(b, j, tolerance);
   }
}

