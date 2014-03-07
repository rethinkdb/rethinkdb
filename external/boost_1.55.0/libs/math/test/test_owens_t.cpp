// test_owens_t.cpp

// Copyright Paul A. Bristow 2012.
// Copyright Benjamin Sobotta 2012.

// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Tested using some 30 decimal digit accuracy values from:
// Fast and accurate calculation of Owen's T-function
// Mike Patefield, and David Tandy
// Journal of Statistical Software, 5 (5), 1-25 (2000).
// http://www.jstatsoft.org/v05/a05/paper  Table 3, page 15
// Values of T(h,a) accurate to thirty figures were calculated using 128 bit arithmetic by
// evaluating (9) with m = 48, the summation over k being continued until additional terms did
// not alter the result. The resultant values Tacc(h,a) say, were validated by evaluating (8) with
// m = 48 (i.e. 96 point Gaussian quadrature).

#define BOOST_MATH_OVERFLOW_ERROR_POLICY ignore_error

#ifdef _MSC_VER
#  pragma warning (disable : 4127) // conditional expression is constant
#  pragma warning (disable : 4305) // 'initializing' : truncation from 'double' to 'const float'
// ?? TODO get rid of these warnings?
#endif

#include <boost/math/concepts/real_concept.hpp> // for real_concept.
using ::boost::math::concepts::real_concept;

#include <boost/math/special_functions/owens_t.hpp> // for owens_t function.
using boost::math::owens_t;
#include <boost/math/distributions/normal.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/array.hpp>

#include "libs/math/test/handle_test_result.hpp"
#include "libs/math/test/table_type.hpp"
#include "libs/math/test/functor.hpp"

//
// Defining TEST_CPP_DEC_FLOAT enables testing of multiprecision support.
// This requires the multiprecision library from sandbox/big_number.
// Note that these tests *do not pass*, but they do give an idea of the 
// error rates that can be expected....
//
#ifdef TEST_CPP_DEC_FLOAT
#include <boost/multiprecision/cpp_dec_float.hpp>

template <class R>
inline R convert_to(const char* s)
{
   try{
      return boost::lexical_cast<R>(s);
   }
   catch(const boost::bad_lexical_cast&)
   {
      return 0;
   }
}

#define SC_(x) convert_to<T>(BOOST_STRINGIZE(x))
#endif

#include "owens_t_T7.hpp"

#include <iostream>
using std::cout;
using std::endl;
#include <limits>
using std::numeric_limits;

void expected_results()
{
   //
   // Define the max and mean errors expected for
   // various compilers and platforms.
   //
   const char* largest_type;
#ifndef BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
   if(boost::math::policies::digits<double, boost::math::policies::policy<> >() == boost::math::policies::digits<long double, boost::math::policies::policy<> >())
   {
      largest_type = "(long\\s+)?double|real_concept";
   }
   else
   {
      largest_type = "long double|real_concept";
   }
#else
   largest_type = "(long\\s+)?double";
#endif

   //
   // Catch all cases come last:
   //
   if(std::numeric_limits<long double>::digits > 60)
   {
      add_expected_result(
         ".*",                            // compiler
         ".*",                            // stdlib
         ".*",                            // platform
         largest_type,                    // test type(s)
         ".*",      // test data group
         "boost::math::owens_t", 500, 100);  // test function
   }
   else
   {
      add_expected_result(
         ".*",                            // compiler
         ".*",                            // stdlib
         ".*",                            // platform
         largest_type,                    // test type(s)
         ".*",      // test data group
         "boost::math::owens_t", 60, 5);  // test function
   }
   //
   // Finish off by printing out the compiler/stdlib/platform names,
   // we do this to make it easier to mark up expected error rates.
   //
   std::cout << "Tests run with " << BOOST_COMPILER << ", " 
      << BOOST_STDLIB << ", " << BOOST_PLATFORM << std::endl;
}


template <class RealType>
void test_spot(
     RealType h,    //
     RealType a,    //
     RealType tol)   // Test tolerance
{
  BOOST_CHECK_CLOSE_FRACTION(owens_t(h, a), 3.89119302347013668966224771378e-2L, tol);
}


template <class RealType> // Any floating-point type RealType.
void test_spots(RealType)
{
  // Basic sanity checks, test data is as accurate as long double,
  // so set tolerance to a few epsilon expressed as a fraction.
  RealType tolerance = boost::math::tools::epsilon<RealType>() * 30; // most OK with 3 eps tolerance.
  cout << "Tolerance = " << tolerance << "." << endl;

  using  ::boost::math::owens_t;
  using ::boost::math::normal_distribution;
  BOOST_MATH_STD_USING // ADL of std names.

  // Checks of six sub-methods T1 to T6.
  BOOST_CHECK_CLOSE_FRACTION(owens_t(static_cast<RealType>(0.0625L), static_cast<RealType>(0.25L)), static_cast<RealType>(3.89119302347013668966224771378e-2L), tolerance);  // T1
  BOOST_CHECK_CLOSE_FRACTION(owens_t(static_cast<RealType>(6.5L), static_cast<RealType>(0.4375L)), static_cast<RealType>(2.00057730485083154100907167685E-11L), tolerance); // T2
  BOOST_CHECK_CLOSE_FRACTION(owens_t(static_cast<RealType>(7L), static_cast<RealType>( 0.96875L)), static_cast<RealType>(6.39906271938986853083219914429E-13L), tolerance); // T3
  BOOST_CHECK_CLOSE_FRACTION(owens_t(static_cast<RealType>(4.78125L), static_cast<RealType>(0.0625L)), static_cast<RealType>(1.06329748046874638058307112826E-7L), tolerance); // T4
  BOOST_CHECK_CLOSE_FRACTION(owens_t(static_cast<RealType>(2.L), static_cast<RealType>(0.5L)), static_cast<RealType>(8.62507798552150713113488319155E-3L), tolerance); // T5
  BOOST_CHECK_CLOSE_FRACTION(owens_t(static_cast<RealType>(1.L), static_cast<RealType>(0.9999975L)), static_cast<RealType>(6.67418089782285927715589822405E-2L), tolerance); // T6
  //BOOST_CHECK_CLOSE_FRACTION(owens_t(static_cast<RealType>(L), static_cast<RealType>(L)), static_cast<RealType>(L), tolerance);

//   BOOST_CHECK_CLOSE_FRACTION(owens_t(static_cast<RealType>(L), static_cast<RealType>(L)), static_cast<RealType>(L), tolerance);

  // Spots values using Mathematica 
  BOOST_CHECK_CLOSE_FRACTION(owens_t(static_cast<RealType>(6.5L), static_cast<RealType>(0.4375L)), static_cast<RealType>(2.00057730485083154100907167684918851101649922551817956120806662022118024594547E-11L), tolerance);
  BOOST_CHECK_CLOSE_FRACTION(owens_t(static_cast<RealType>(0.4375L), static_cast<RealType>(6.5L)), static_cast<RealType>(0.16540130125449396247498691826626273249659241838438244251206819782787761751256L), tolerance);
  BOOST_CHECK_CLOSE_FRACTION(owens_t(static_cast<RealType>(7.L), static_cast<RealType>(0.96875L)), static_cast<RealType>(6.39906271938986853083219914428916013764797190941459233223182225724846022843930e-13L), tolerance);
  BOOST_CHECK_CLOSE_FRACTION(owens_t(static_cast<RealType>(0.96875L), static_cast<RealType>(7.L)), static_cast<RealType>(0.08316748474602973770533230453272140919966614259525787470390475393923633179072L), tolerance);
  BOOST_CHECK_CLOSE_FRACTION(owens_t(static_cast<RealType>(4.78125L), static_cast<RealType>(0.0625L)), static_cast<RealType>(1.06329748046874638058307112826015825291136503488102191050906959246644942646701e-7L), tolerance);
  BOOST_CHECK_CLOSE_FRACTION(owens_t(static_cast<RealType>(0.0625L), static_cast<RealType>(4.78125L)), static_cast<RealType>(0.21571185819897989857261253680409017017649352928888660746045361855686569265171L), tolerance);
  BOOST_CHECK_CLOSE_FRACTION(owens_t(static_cast<RealType>(2.L), static_cast<RealType>(0.5L)), static_cast<RealType>(0.00862507798552150713113488319154637187875641190390854291100809449487812876461L), tolerance);
  BOOST_CHECK_CLOSE_FRACTION(owens_t(static_cast<RealType>(0.5L), static_cast<RealType>(2L)), static_cast<RealType>(0.14158060365397839346662819588111542648867283386549027383784843786494855594607L), tolerance);

  // check basic properties
  BOOST_CHECK_EQUAL(owens_t(static_cast<RealType>(0.5L), static_cast<RealType>(2L)), owens_t(static_cast<RealType>(-0.5L), static_cast<RealType>(2L)));
  BOOST_CHECK_EQUAL(owens_t(static_cast<RealType>(0.5L), static_cast<RealType>(2L)), -owens_t(static_cast<RealType>(0.5L), static_cast<RealType>(-2L)));
  BOOST_CHECK_EQUAL(owens_t(static_cast<RealType>(0.5L), static_cast<RealType>(2L)), -owens_t(static_cast<RealType>(-0.5L), static_cast<RealType>(-2L)));

  // Special relations from Owen's original paper:
  BOOST_CHECK_EQUAL(owens_t(static_cast<RealType>(0.5), static_cast<RealType>(0)), static_cast<RealType>(0));
  BOOST_CHECK_EQUAL(owens_t(static_cast<RealType>(10), static_cast<RealType>(0)), static_cast<RealType>(0));
  BOOST_CHECK_EQUAL(owens_t(static_cast<RealType>(10000), static_cast<RealType>(0)), static_cast<RealType>(0));

  BOOST_CHECK_CLOSE_FRACTION(owens_t(static_cast<RealType>(0), static_cast<RealType>(2L)), atan(static_cast<RealType>(2L)) / (boost::math::constants::pi<RealType>() * 2), tolerance);
  BOOST_CHECK_CLOSE_FRACTION(owens_t(static_cast<RealType>(0), static_cast<RealType>(0.5L)), atan(static_cast<RealType>(0.5L)) / (boost::math::constants::pi<RealType>() * 2), tolerance);
  BOOST_CHECK_CLOSE_FRACTION(owens_t(static_cast<RealType>(0), static_cast<RealType>(2000L)), atan(static_cast<RealType>(2000L)) / (boost::math::constants::pi<RealType>() * 2), tolerance);
  BOOST_CHECK_CLOSE_FRACTION(owens_t(static_cast<RealType>(5), static_cast<RealType>(1)), cdf(normal_distribution<RealType>(), 5) * cdf(complement(normal_distribution<RealType>(), 5)) / 2, tolerance);
  BOOST_CHECK_CLOSE_FRACTION(owens_t(static_cast<RealType>(0.125), static_cast<RealType>(1)), cdf(normal_distribution<RealType>(), 0.125) * cdf(complement(normal_distribution<RealType>(), 0.125)) / 2, tolerance);
  if(std::numeric_limits<RealType>::has_infinity)
  {
    BOOST_CHECK_CLOSE_FRACTION(owens_t(static_cast<RealType>(0.125), std::numeric_limits<RealType>::infinity()), cdf(complement(normal_distribution<RealType>(), 0.125)) / 2, tolerance);
    BOOST_CHECK_CLOSE_FRACTION(owens_t(static_cast<RealType>(5), std::numeric_limits<RealType>::infinity()), cdf(complement(normal_distribution<RealType>(), 5)) / 2, tolerance);
    BOOST_CHECK_CLOSE_FRACTION(owens_t(static_cast<RealType>(-0.125), std::numeric_limits<RealType>::infinity()), cdf(normal_distribution<RealType>(), -0.125) / 2, tolerance);
    BOOST_CHECK_CLOSE_FRACTION(owens_t(static_cast<RealType>(-5), std::numeric_limits<RealType>::infinity()), cdf(normal_distribution<RealType>(), -5) / 2, tolerance);
  }
} // template <class RealType>void test_spots(RealType)

template <class RealType> // Any floating-point type RealType.
void check_against_T7(RealType)
{
  // Basic sanity checks, test data is as accurate as long double,
  // so set tolerance to a few epsilon expressed as a fraction.
  RealType tolerance = boost::math::tools::epsilon<RealType>() * 150; // most OK with 3 eps tolerance.
  cout << "Tolerance = " << tolerance << "." << endl;

  using  ::boost::math::owens_t;
  using namespace std; // ADL of std names.

  // apply log scale because points near zero are more interesting
  for(RealType a = static_cast<RealType>(-10.0l); a < static_cast<RealType>(3l); a+= static_cast<RealType>(0.2l))
    for(RealType h = static_cast<RealType>(-10.0l); h < static_cast<RealType>(3.5l); h+= static_cast<RealType>(0.2l))
    {
      const RealType expa = exp(a);
      const RealType exph = exp(h);
      const RealType t = boost::math::owens_t(exph, expa);
      RealType t7 = boost::math::owens_t_T7(exph,expa);
      //if(!(boost::math::isnormal)(t) || !(boost::math::isnormal)(t7))
      //   std::cout << "a = " << expa << " h = " << exph << " t = " << t << " t7 = " << t7 << std::endl;
      BOOST_CHECK_CLOSE_FRACTION(t, t7, tolerance);
    }
    
} // template <class RealType>void test_spots(RealType)

template <class Real, class T>
void do_test_owens_t(const T& data, const char* type_name, const char* test_name)
{
   typedef typename T::value_type row_type;
   typedef Real                   value_type;

   typedef value_type (*pg)(value_type, value_type);
#if defined(BOOST_MATH_NO_DEDUCED_FUNCTION_POINTERS)
   pg funcp = boost::math::owens_t<value_type>;
#else
   pg funcp = boost::math::owens_t;
#endif

   boost::math::tools::test_result<value_type> result;

   std::cout << "Testing " << test_name << " with type " << type_name
      << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";

   //
   // test hermite against data:
   //
   result = boost::math::tools::test_hetero<Real>(
      data, 
      bind_func<Real>(funcp, 0, 1), 
      extract_result<Real>(2));
   handle_test_result(result, data[result.worst()], result.worst(), type_name, "boost::math::owens_t", test_name);

   std::cout << std::endl;
}

template <class T>
void test_owens_t(T, const char* name)
{
   //
   // The actual test data is rather verbose, so it's in a separate file
   //
   // The contents are as follows, each row of data contains
   // three items, input value a, input value b and erf(a, b):
   // 
#  include "owens_t.ipp"

   do_test_owens_t<T>(owens_t, name, "Owens T (medium small values)");

#include "owens_t_large_data.ipp"

   do_test_owens_t<T>(owens_t_large_data, name, "Owens T (large and diverse values)");
}


BOOST_AUTO_TEST_CASE( test_main )
{
  BOOST_MATH_CONTROL_FP;

  expected_results();

  // Basic sanity-check spot values.

  // (Parameter value, arbitrarily zero, only communicates the floating point type).
  test_spots(0.0F); // Test float.
  test_spots(0.0); // Test double.
#ifndef BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
  test_spots(0.0L); // Test long double.
#if !BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x582))
  test_spots(boost::math::concepts::real_concept(0.)); // Test real concept.
#endif
#endif

  check_against_T7(0.0F); // Test float.
  check_against_T7(0.0); // Test double.
#ifndef BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
  check_against_T7(0.0L); // Test long double.
#if !BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x582))
  check_against_T7(boost::math::concepts::real_concept(0.)); // Test real concept.
#endif
#endif

  test_owens_t(0.0F, "float"); // Test float.
  test_owens_t(0.0, "double"); // Test double.
#ifndef BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
  test_owens_t(0.0L, "long double"); // Test long double.
#if !BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x582))
  test_owens_t(boost::math::concepts::real_concept(0.), "real_concept"); // Test real concept.
#endif
#endif
#ifdef TEST_CPP_DEC_FLOAT
  typedef boost::multiprecision::mp_number<boost::multiprecision::cpp_dec_float<35> > cpp_dec_float_35;
  test_owens_t(cpp_dec_float_35(0), "cpp_dec_float_35"); // Test real concept.
  test_owens_t(boost::multiprecision::cpp_dec_float_50(0), "cpp_dec_float_50"); // Test real concept.
  test_owens_t(boost::multiprecision::cpp_dec_float_100(0), "cpp_dec_float_100"); // Test real concept.
#endif
  
} // BOOST_AUTO_TEST_CASE( test_main )

/*

Output:

  Description: Autorun "J:\Cpp\MathToolkit\test\Math_test\Debug\test_owens_t.exe"
  Running 1 test case...
  Tests run with Microsoft Visual C++ version 10.0, Dinkumware standard library version 520, Win32
  Tolerance = 3.57628e-006.
  Tolerance = 6.66134e-015.
  Tolerance = 6.66134e-015.
  Tolerance = 6.66134e-015.
  Tolerance = 1.78814e-005.
  Tolerance = 3.33067e-014.
  Tolerance = 3.33067e-014.
  Tolerance = 3.33067e-014.
  Testing Owens T (medium small values) with type float
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  boost::math::owens_t<float> Max = 0 RMS Mean=0
  
  
  Testing Owens T (large and diverse values) with type float
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  boost::math::owens_t<float> Max = 0 RMS Mean=0
  
  
  Testing Owens T (medium small values) with type double
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  boost::math::owens_t<double> Max = 4.375 RMS Mean=0.9728
      worst case at row: 81
      { 4.4206809997558594, 0.1269868016242981, 1.0900281236140834e-006 }
  
  
  Testing Owens T (large and diverse values) with type double
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  boost::math::owens_t<double> Max = 3.781 RMS Mean=0.6206
      worst case at row: 430
      { 3.4516773223876953, 0.0010718167759478092, 4.413983645332431e-007 }
  
  
  Testing Owens T (medium small values) with type long double
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  boost::math::owens_t<long double> Max = 4.375 RMS Mean=0.9728
      worst case at row: 81
      { 4.4206809997558594, 0.1269868016242981, 1.0900281236140834e-006 }
  
  
  Testing Owens T (large and diverse values) with type long double
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  boost::math::owens_t<long double> Max = 3.781 RMS Mean=0.6206
      worst case at row: 430
      { 3.4516773223876953, 0.0010718167759478092, 4.413983645332431e-007 }
  
  
  Testing Owens T (medium small values) with type real_concept
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  boost::math::owens_t<real_concept> Max = 4.375 RMS Mean=1.032
      worst case at row: 81
      { 4.4206809997558594, 0.1269868016242981, 1.0900281236140834e-006 }
  
  
  Testing Owens T (large and diverse values) with type real_concept
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  boost::math::owens_t<real_concept> Max = 21.04 RMS Mean=1.102
      worst case at row: 439
      { 3.4516773223876953, 0.98384737968444824, 0.00013923002576038691 }
  
  
  
  *** No errors detected


*/



