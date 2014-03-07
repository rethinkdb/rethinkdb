// Copyright John Maddock 2006.
// Copyright Paul A. Bristow 2007, 2009
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_MATH_OVERFLOW_ERROR_POLICY ignore_error

#include <boost/math/concepts/real_concept.hpp>
#include <boost/math/special_functions/math_fwd.hpp>
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/math/tools/stats.hpp>
#include <boost/math/tools/test.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/type_traits/is_floating_point.hpp>
#include <boost/array.hpp>
#include "functor.hpp"

#include "test_gamma_hooks.hpp"
#include "handle_test_result.hpp"
#include "table_type.hpp"

#ifndef SC_
#define SC_(x) static_cast<typename table_type<T>::type>(BOOST_JOIN(x, L))
#endif

template <class Real, class T>
void do_test_gamma(const T& data, const char* type_name, const char* test_name)
{
   typedef Real                   value_type;

   typedef value_type (*pg)(value_type);
#if defined(BOOST_MATH_NO_DEDUCED_FUNCTION_POINTERS)
   pg funcp = boost::math::tgamma<value_type>;
#else
   pg funcp = boost::math::tgamma;
#endif

   boost::math::tools::test_result<value_type> result;

   std::cout << "Testing " << test_name << " with type " << type_name
      << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";

   //
   // test tgamma against data:
   //
   result = boost::math::tools::test_hetero<Real>(
      data,
      bind_func<Real>(funcp, 0),
      extract_result<Real>(1));
   handle_test_result(result, data[result.worst()], result.worst(), type_name, "boost::math::tgamma", test_name);
#ifdef TEST_OTHER
   if(::boost::is_floating_point<value_type>::value){
      funcp = other::tgamma;
      result = boost::math::tools::test_hetero<Real>(
         data,
         bind_func<Real>(funcp, 0),
         extract_result<Real>(1));
      print_test_result(result, data[result.worst()], result.worst(), type_name, "other::tgamma");
   }
#endif
   //
   // test lgamma against data:
   //
#if defined(BOOST_MATH_NO_DEDUCED_FUNCTION_POINTERS)
   funcp = boost::math::lgamma<value_type>;
#else
   funcp = boost::math::lgamma;
#endif
   result = boost::math::tools::test_hetero<Real>(
      data,
      bind_func<Real>(funcp, 0),
      extract_result<Real>(2));
   handle_test_result(result, data[result.worst()], result.worst(), type_name, "boost::math::lgamma", test_name);
#ifdef TEST_OTHER
   if(::boost::is_floating_point<value_type>::value){
      funcp = other::lgamma;
      result = boost::math::tools::test_hetero<Real>(
         data,
         bind_func<Real>(funcp, 0),
         extract_result<Real>(2));
      print_test_result(result, data[result.worst()], result.worst(), type_name, "other::lgamma");
   }
#endif

   std::cout << std::endl;
}

template <class Real, class T>
void do_test_gammap1m1(const T& data, const char* type_name, const char* test_name)
{
   typedef Real                   value_type;

   typedef value_type (*pg)(value_type);
#if defined(BOOST_MATH_NO_DEDUCED_FUNCTION_POINTERS)
   pg funcp = boost::math::tgamma1pm1<value_type>;
#else
   pg funcp = boost::math::tgamma1pm1;
#endif

   boost::math::tools::test_result<value_type> result;

   std::cout << "Testing " << test_name << " with type " << type_name
      << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";

   //
   // test tgamma1pm1 against data:
   //
   result = boost::math::tools::test_hetero<Real>(
      data,
      bind_func<Real>(funcp, 0),
      extract_result<Real>(1));
   handle_test_result(result, data[result.worst()], result.worst(), type_name, "boost::math::tgamma1pm1", test_name);
   std::cout << std::endl;
}

template <class T>
void test_gamma(T, const char* name)
{
   //
   // The actual test data is rather verbose, so it's in a separate file
   //
   // The contents are as follows, each row of data contains
   // three items, input value, gamma and lgamma:
   //
   // gamma and lgamma at integer and half integer values:
   // boost::array<boost::array<T, 3>, N> factorials;
   //
   // gamma and lgamma for z near 0:
   // boost::array<boost::array<T, 3>, N> near_0;
   //
   // gamma and lgamma for z near 1:
   // boost::array<boost::array<T, 3>, N> near_1;
   //
   // gamma and lgamma for z near 2:
   // boost::array<boost::array<T, 3>, N> near_2;
   //
   // gamma and lgamma for z near -10:
   // boost::array<boost::array<T, 3>, N> near_m10;
   //
   // gamma and lgamma for z near -55:
   // boost::array<boost::array<T, 3>, N> near_m55;
   //
   // The last two cases are chosen more or less at random,
   // except that one is even and the other odd, and both are
   // at negative poles.  The data near zero also tests near
   // a pole, the data near 1 and 2 are to probe lgamma as
   // the result -> 0.
   //
#  include "test_gamma_data.ipp"

   do_test_gamma<T>(factorials, name, "factorials");
   do_test_gamma<T>(near_0, name, "near 0");
   do_test_gamma<T>(near_1, name, "near 1");
   do_test_gamma<T>(near_2, name, "near 2");
   do_test_gamma<T>(near_m10, name, "near -10");
   do_test_gamma<T>(near_m55, name, "near -55");

   //
   // And now tgamma1pm1 which computes gamma(1+dz)-1:
   //
   do_test_gammap1m1<T>(gammap1m1_data, name, "tgamma1pm1(dz)");
}

template <class T>
void test_spots(T)
{
   //
   // basic sanity checks, tolerance is 50 epsilon expressed as a percentage:
   //
   T tolerance = boost::math::tools::epsilon<T>() * 5000;
   BOOST_CHECK_CLOSE(::boost::math::tgamma(static_cast<T>(3.5)), static_cast<T>(3.3233509704478425511840640312646472177454052302295L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::tgamma(static_cast<T>(0.125)), static_cast<T>(7.5339415987976119046992298412151336246104195881491L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::tgamma(static_cast<T>(-0.125)), static_cast<T>(-8.7172188593831756100190140408231437691829605421405L), tolerance);
   BOOST_CHECK_CLOSE(::boost::math::tgamma(static_cast<T>(-3.125)), static_cast<T>(1.1668538708507675587790157356605097019141636072094L), tolerance);
   // Lower tolerance on this one, is only really needed on Linux x86 systems, result is mostly down to std lib accuracy:
   BOOST_CHECK_CLOSE(::boost::math::tgamma(static_cast<T>(-53249.0/1024)), static_cast<T>(-1.2646559519067605488251406578743995122462767733517e-65L), tolerance * 3);

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4127)
#endif
   // Test bug fixes in tgamma:
   if(std::numeric_limits<T>::max_exponent10 > 244)
   {
      BOOST_CHECK_CLOSE(::boost::math::tgamma(static_cast<T>(142.75)), static_cast<T>(7.8029496083318133344429227511387928576820621466e244L), tolerance * 4);
   }
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

   int sign = 1;
   BOOST_CHECK_CLOSE(::boost::math::lgamma(static_cast<T>(3.5), &sign), static_cast<T>(1.2009736023470742248160218814507129957702389154682L), tolerance);
   BOOST_CHECK(sign == 1);
   BOOST_CHECK_CLOSE(::boost::math::lgamma(static_cast<T>(0.125), &sign), static_cast<T>(2.0194183575537963453202905211670995899482809521344L), tolerance);
   BOOST_CHECK(sign == 1);
   BOOST_CHECK_CLOSE(::boost::math::lgamma(static_cast<T>(-0.125), &sign), static_cast<T>(2.1653002489051702517540619481440174064962195287626L), tolerance);
   BOOST_CHECK(sign == -1);
   BOOST_CHECK_CLOSE(::boost::math::lgamma(static_cast<T>(-3.125), &sign), static_cast<T>(0.1543111276840418242676072830970532952413339012367L), tolerance);
   BOOST_CHECK(sign == 1);
   BOOST_CHECK_CLOSE(::boost::math::lgamma(static_cast<T>(-53249.0/1024), &sign), static_cast<T>(-149.43323093420259741100038126078721302600128285894L), tolerance);
   BOOST_CHECK(sign == -1);
}

