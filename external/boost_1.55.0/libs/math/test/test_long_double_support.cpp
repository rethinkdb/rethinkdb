// Copyright John Maddock 2009

// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <cmath>
#include <math.h>
#include <limits.h>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp> // Boost.Test
#include <boost/test/results_collector.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>

#include <iostream>
#include <iomanip>
   using std::cout;
   using std::endl;
   using std::setprecision;

#include <boost/array.hpp>
#include "functor.hpp"
#include "handle_test_result.hpp"
#include "table_type.hpp"

#include <boost/math/tools/config.hpp>

void expected_results()
{
   //
   // Define the max and mean errors expected for
   // various compilers and platforms.
   //
   const char* largest_type;
   largest_type = "(long\\s+)?double";

   add_expected_result(
      ".*",                          // compiler
      ".*",                          // stdlib
      ".*",                          // platform
      largest_type,                  // test type(s)
      ".*",                          // test data group
      ".*", 50, 20);                 // test function
}

template <class A>
void do_test_std_function(const A& data, const char* type_name, const char* function_name, const char* test_name, long double (*proc)(long double), const char* inv_function_name = 0, long double (*inv_proc)(long double) = 0)
{
   // warning suppression:
   (void)data;
   (void)type_name;
   (void)test_name;
   typedef typename A::value_type row_type;
   typedef typename row_type::value_type value_type;

   boost::math::tools::test_result<value_type> result;

   //
   // test against data:
   //
   result = boost::math::tools::test(
      data,
      bind_func<value_type>(proc, 0),
      extract_result<value_type>(1));
   handle_test_result(result, data[result.worst()], result.worst(), type_name, function_name, test_name);
   if(inv_proc)
   {
      result = boost::math::tools::test(
         data,
         bind_func<value_type>(inv_proc, 1),
         extract_result<value_type>(0));
      handle_test_result(result, data[result.worst()], result.worst(), type_name, inv_function_name, test_name);
   }
}


void test_spots()
{
   // Basic sanity checks.
   // Test data taken from functions.wolfram.com
   long double (*unary_proc)(long double);
   long double (*inv_unary_proc)(long double);
   //
   // COS:
   //
   boost::array<boost::array<long double, 2>, 4> cos_test_data = {{
      {{ 0, 1, }},
      {{ 0.125L, 0.992197667229329053149096907788250869543327304736601263468910L, }},
      {{ 1.125L, 0.431176516798666176551969042921689826840697850225767471037314L, }},
      {{ 1.75L, -0.178246055649492090382676943942631358969920851291548272886063L, }},
   }};
   unary_proc = std::cos;
   inv_unary_proc = std::acos;
   do_test_std_function(cos_test_data, "long double", "std::cos", "Mathematica data", unary_proc, "std::acos", inv_unary_proc);
   unary_proc = ::cosl;
   inv_unary_proc = ::acosl;
   do_test_std_function(cos_test_data, "long double", "::cosl", "Mathematica data", unary_proc, "::acosl", inv_unary_proc);
   //
   // SIN:
   //
   boost::array<boost::array<long double, 2>, 6> sin_test_data = {{
      {{ 0, 0, }},
      {{ 0.125L, 0.124674733385227689957442708712108467587834905641679257885515L, }},
      {{ -0.125L, -0.124674733385227689957442708712108467587834905641679257885515L, }},
      {{ 1.125L, 0.902267594099095162918416128654829100758989018716070814389152L, }},
#if LDBL_MAX_EXP > DBL_MAX_EXP
      {{ 1e-500L, 1e-500L, }},
      {{ 1e-1500L, 1e-1500L, }},
#else
      {{ 0, 0, }},
      {{ 0, 0, }},
#endif
   }};
   unary_proc = std::sin;
   inv_unary_proc = std::asin;
   do_test_std_function(sin_test_data, "long double", "std::sin", "Mathematica data", unary_proc, "std::asin", inv_unary_proc);
   unary_proc = ::sinl;
   inv_unary_proc = ::asinl;
   do_test_std_function(sin_test_data, "long double", "::sinl", "Mathematica data", unary_proc, "::asinl", inv_unary_proc);
   //
   // TAN:
   //
   boost::array<boost::array<long double, 2>, 6> tan_test_data = {{
      {{ 0, 0, }},
      {{ 0.125L, 0.125655136575130967792678218629774000758665763892225542668867L, }},
      {{ -0.125L, -0.125655136575130967792678218629774000758665763892225542668867L, }},
      {{ 1.125L, 2.09257127637217900442373398123488678225994171614872057291399L, }},
#if LDBL_MAX_EXP > DBL_MAX_EXP
      {{ 1e-500L, 1e-500L, }},
      {{ 1e-1500L, 1e-1500L, }},
#else
      {{ 0, 0, }},
      {{ 0, 0, }},
#endif
   }};
   unary_proc = std::tan;
   inv_unary_proc = std::atan;
   do_test_std_function(tan_test_data, "long double", "std::tan", "Mathematica data", unary_proc, "std::atan", inv_unary_proc);
   unary_proc = ::tanl;
   inv_unary_proc = ::atanl;
   do_test_std_function(tan_test_data, "long double", "::tanl", "Mathematica data", unary_proc, "::atanl", inv_unary_proc);
   //
   // EXP:
   //
   boost::array<boost::array<long double, 2>, 16> exp_test_data = {{
      {{ 0, 1, }},
      {{ 0.125L, 1.13314845306682631682900722781179387256550313174518162591282L, }},
      {{ -0.125L, 0.882496902584595402864892143229050736222004824990650741770309L, }},
      {{ 1.125L, 3.08021684891803124500466787877703957705899375982613074033239L, }},
      {{ 4.60517018598809136803598290936872841520220297725754595206666L, 100L, }},
      {{ 23.0258509299404568401799145468436420760110148862877297603333L, 1e10L, }},
      {{ 230.258509299404568401799145468436420760110148862877297603333L, 1e100L, }},
      {{ -230.258509299404568401799145468436420760110148862877297603333L, 1e-100L, }},
      {{ -23.0258509299404568401799145468436420760110148862877297603333L, 1e-10L, }},
      {{ -4.60517018598809136803598290936872841520220297725754595206666L, 0.01L, }},
#if LDBL_MAX_EXP > DBL_MAX_EXP
      {{ 1151.5L, 1.23054049904018215810329849694e+500L, }},
      {{ 2302.5, 9.1842687219959504902800771504e+999L, }},
      {{ 11351.5, 7.83089362896060182981072520459e+4929L, }},
      {{ -11351.5, 1.27699346636729947157842192471e-4930L, }},
      {{ -2302.5, 1.0888183156107362404277325218e-1000L, }},
      {{ -1151.5, 8.12651026747999724274336150307e-501L, }},
#else
      {{ 0, 1, }},
      {{ 0, 1, }},
      {{ 0, 1, }},
      {{ 0, 1, }},
      {{ 0, 1, }},
      {{ 0, 1, }},
#endif
   }};
   unary_proc = std::exp;
   inv_unary_proc = std::log;
   do_test_std_function(exp_test_data, "long double", "std::exp", "Mathematica data", unary_proc, "std::log", inv_unary_proc);
   unary_proc = ::expl;
   inv_unary_proc = ::logl;
   do_test_std_function(exp_test_data, "long double", "::expl", "Mathematica data", unary_proc, "::logl", inv_unary_proc);
   //
   // SQRT:
   //
   boost::array<boost::array<long double, 2>, 8> sqrt_test_data = {{
      {{ 1, 1, }},
      {{ 0.125L, 0.353553390593273762200422181052424519642417968844237018294170L, }},
      {{ 1.125L, 1.06066017177982128660126654315727355892725390653271105488251L, }},
      {{ 1e10L, 1e5L, }},
      {{ 1e100L, 1e50L, }},
#if LDBL_MAX_EXP > DBL_MAX_EXP
      {{ 1e500L, 1e250L, }},
      {{ 1e1000L, 1e500L, }},
      {{ 1e4930L, 1e2465L }},
#else
      {{ 1, 1, }},
      {{ 1, 1, }},
      {{ 1, 1, }},
#endif
   }};
   unary_proc = std::sqrt;
   inv_unary_proc = 0;
   do_test_std_function(sqrt_test_data, "long double", "std::sqrt", "Mathematica data", unary_proc, "", inv_unary_proc);
   unary_proc = ::sqrtl;
   do_test_std_function(sqrt_test_data, "long double", "::sqrtl", "Mathematica data", unary_proc, "", inv_unary_proc);
}


BOOST_AUTO_TEST_CASE( test_main )
{
   expected_results();
   std::cout << "Running tests with BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS "
#ifdef BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
      "defined."
#else
      "not defined."
#endif
      << std::endl;
   // Basic sanity-check spot values.
   // (Parameter value, arbitrarily zero, only communicates the floating point type).
   test_spots(); // Test long double.


} // BOOST_AUTO_TEST_CASE( test_main )

