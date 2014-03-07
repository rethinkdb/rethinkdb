///////////////////////////////////////////////////////////////
//  Copyright 2011 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

#include "setup.hpp"
#include "table_type.hpp"
#define TEST_UDT

#define TEST_DATA 1

#include <boost/math/special_functions/math_fwd.hpp>
#include "libs/math/test/test_ibeta.hpp"

void expected_results()
{
   //
   // Define the max and mean errors expected for
   // various compilers and platforms.
   //
   add_expected_result(
      "[^|]*",                          // compiler
      "[^|]*",                          // stdlib
      "[^|]*",                          // platform
      ".*gmp_float<18>.*",              // test type(s)
      "(?i).*small.*",                  // test data group
      ".*", 3000, 1000);                // test function
   add_expected_result(
      "[^|]*",                          // compiler
      "[^|]*",                          // stdlib
      "[^|]*",                          // platform
      ".*mpfr_float_backend<18>.*",     // test type(s)
      "(?i).*small.*",                  // test data group
      ".*", 2000, 500);                  // test function
#ifdef BOOST_INTEL
   add_expected_result(
      "[^|]*",                          // compiler
      "[^|]*",                          // stdlib
      "[^|]*",                          // platform
      "float128",                             // test type(s)
      "(?i).*small.*",                  // test data group
      ".*", 500, 100);  // test function
#endif
   add_expected_result(
      "[^|]*",                          // compiler
      "[^|]*",                          // stdlib
      "[^|]*",                          // platform
      ".*",                             // test type(s)
      "(?i).*small.*",                  // test data group
      ".*", 90, 25);  // test function
   add_expected_result(
      "[^|]*",                          // compiler
      "[^|]*",                          // stdlib
      "[^|]*",                          // platform
      ".*",                             // test type(s)
      "(?i).*medium.*",                 // test data group
      ".*", 150, 50);  // test function
   add_expected_result(
      "[^|]*",                          // compiler
      "[^|]*",                          // stdlib
      "[^|]*",                          // platform
      ".*",                             // test type(s)
      "(?i).*large.*",                  // test data group
      ".*", 5000, 500);                 // test function
   //
   // Finish off by printing out the compiler/stdlib/platform names,
   // we do this to make it easier to mark up expected error rates.
   //
   std::cout << "Tests run with " << BOOST_COMPILER << ", "
      << BOOST_STDLIB << ", " << BOOST_PLATFORM << std::endl;
}

template <class T>
void test(T t, const char* p)
{
   test_beta(t, p);
}


BOOST_AUTO_TEST_CASE( test_main )
{
   using namespace boost::multiprecision;
   expected_results();
   //
   // Test at:
   // 18 decimal digits: tests 80-bit long double approximations
   // 30 decimal digits: tests 128-bit long double approximations
   // 35 decimal digits: tests arbitrary precision code
   //
   ALL_TESTS
}

