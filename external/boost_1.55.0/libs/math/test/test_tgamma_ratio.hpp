// Copyright John Maddock 2006.
// Copyright Paul A. Bristow 2007, 2009
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_MATH_OVERFLOW_ERROR_POLICY ignore_error

#include <boost/math/concepts/real_concept.hpp>
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/math/special_functions/gamma.hpp>
#include <boost/math/tools/stats.hpp>
#include <boost/math/tools/test.hpp>
#include <boost/array.hpp>
#include "functor.hpp"

#include "handle_test_result.hpp"
#include "table_type.hpp"

#ifndef SC_
#define SC_(x) static_cast<typename table_type<T>::type>(BOOST_JOIN(x, L))
#endif

template <class Real>
struct negative_tgamma_ratio
{
   template <class Row>
   Real operator()(const Row& row)
   {
      return boost::math::tgamma_delta_ratio(Real(row[0]), -Real(row[1]));
   }
};

template <class Real, class T>
void do_test_tgamma_delta_ratio(const T& data, const char* type_name, const char* test_name)
{
   typedef Real                   value_type;

   typedef value_type (*pg)(value_type, value_type);
#if defined(BOOST_MATH_NO_DEDUCED_FUNCTION_POINTERS)
   pg funcp = boost::math::tgamma_delta_ratio<value_type, value_type>;
#else
   pg funcp = boost::math::tgamma_delta_ratio;
#endif

   boost::math::tools::test_result<value_type> result;

   std::cout << "Testing " << test_name << " with type " << type_name
      << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";

   //
   // test tgamma_delta_ratio against data:
   //
   result = boost::math::tools::test_hetero<Real>(
      data,
      bind_func<Real>(funcp, 0, 1),
      extract_result<Real>(2));
   handle_test_result(result, data[result.worst()], result.worst(), type_name, "boost::math::tgamma_delta_ratio(a, delta)", test_name);
   result = boost::math::tools::test_hetero<Real>(
      data,
      negative_tgamma_ratio<Real>(),
      extract_result<Real>(3));
   handle_test_result(result, data[result.worst()], result.worst(), type_name, "boost::math::tgamma_delta_ratio(a -delta)", test_name);
}

template <class Real, class T>
void do_test_tgamma_ratio(const T& data, const char* type_name, const char* test_name)
{
   typedef Real                   value_type;

   typedef value_type (*pg)(value_type, value_type);
#if defined(BOOST_MATH_NO_DEDUCED_FUNCTION_POINTERS)
   pg funcp = boost::math::tgamma_ratio<value_type, value_type>;
#else
   pg funcp = boost::math::tgamma_ratio;
#endif

   boost::math::tools::test_result<value_type> result;

   std::cout << "Testing " << test_name << " with type " << type_name
      << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";

   //
   // test tgamma_ratio against data:
   //
   result = boost::math::tools::test_hetero<Real>(
      data,
      bind_func<Real>(funcp, 0, 1),
      extract_result<Real>(2));
   handle_test_result(result, data[result.worst()], result.worst(), type_name, "boost::math::tgamma_ratio(a, b)", test_name);
}

template <class T>
void test_tgamma_ratio(T, const char* name)
{
   //
   // The actual test data is rather verbose, so it's in a separate file
   //
#  include "tgamma_delta_ratio_data.ipp"

   do_test_tgamma_delta_ratio<T>(tgamma_delta_ratio_data, name, "tgamma + small delta ratios");

#  include "tgamma_delta_ratio_int.ipp"

   do_test_tgamma_delta_ratio<T>(tgamma_delta_ratio_int, name, "tgamma + small integer ratios");

#  include "tgamma_delta_ratio_int2.ipp"

   do_test_tgamma_delta_ratio<T>(tgamma_delta_ratio_int2, name, "integer tgamma ratios");

#  include "tgamma_ratio_data.ipp"

   do_test_tgamma_ratio<T>(tgamma_ratio_data, name, "tgamma ratios");

}

template <class T>
void test_spots(T, const char*)
{
#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable:4127 4756)
#endif
   //
   // A few special spot tests:
   //
   BOOST_MATH_STD_USING
   T tol = boost::math::tools::epsilon<T>() * 20;
   if(std::numeric_limits<T>::max_exponent > 200)
   {
      BOOST_CHECK_CLOSE_FRACTION(boost::math::tgamma_ratio(ldexp(T(1), -500), T(180.25)), T(8.0113754557649679470816892372669519037339812035512e-178L), 3 * tol);
      BOOST_CHECK_CLOSE_FRACTION(boost::math::tgamma_ratio(ldexp(T(1), -525), T(192.25)), T(1.5966560279353205461166489184101261541784867035063e-197L), 3 * tol);
      BOOST_CHECK_CLOSE_FRACTION(boost::math::tgamma_ratio(T(182.25), ldexp(T(1), -500)), T(4.077990437521002194346763299159975185747917450788e+181L), 3 * tol);
      BOOST_CHECK_CLOSE_FRACTION(boost::math::tgamma_ratio(T(193.25), ldexp(T(1), -525)), T(1.2040790040958522422697601672703926839178050326148e+199L), 3 * tol);
      BOOST_CHECK_CLOSE_FRACTION(boost::math::tgamma_ratio(T(193.25), T(194.75)), T(0.00037151765099653237632823607820104961270831942138159L), 3 * tol);
   }
   BOOST_CHECK_THROW(boost::math::tgamma_ratio(0, 2), std::domain_error);
   BOOST_CHECK_THROW(boost::math::tgamma_ratio(2, 0), std::domain_error);
   BOOST_CHECK_THROW(boost::math::tgamma_ratio(-1, 2), std::domain_error);
   BOOST_CHECK_THROW(boost::math::tgamma_ratio(2, -1), std::domain_error);
   if(std::numeric_limits<T>::has_denorm && (std::numeric_limits<T>::max_exponent == std::numeric_limits<long double>::max_exponent))
   {
      BOOST_CHECK_THROW(boost::math::tgamma_ratio(std::numeric_limits<T>::denorm_min(), 2), std::domain_error);
      BOOST_CHECK_THROW(boost::math::tgamma_ratio(2, std::numeric_limits<T>::denorm_min()), std::domain_error);
   }
   if(std::numeric_limits<T>::has_infinity)
   {
      BOOST_CHECK_THROW(boost::math::tgamma_ratio(std::numeric_limits<T>::infinity(), 2), std::domain_error);
      BOOST_CHECK_THROW(boost::math::tgamma_ratio(2, std::numeric_limits<T>::infinity()), std::domain_error);
   }
#ifdef _MSC_VER
# pragma warning(pop)
#endif
}

