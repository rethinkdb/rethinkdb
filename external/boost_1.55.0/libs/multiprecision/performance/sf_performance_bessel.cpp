///////////////////////////////////////////////////////////////
//  Copyright 2011 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

#include "sf_performance.hpp"

void bessel_tests_1();
void bessel_tests_2();
void bessel_tests_3();
void bessel_tests_4();
void bessel_tests_5();
void bessel_tests_6();

void bessel_tests()
{
   //
   // 50 digits first:
   //
   std::cout << "Testing Bessel Functions at 50 digits....." << std::endl;
#if defined(TEST_MPFR) || defined(TEST_MPFR_CLASS)
   mpfr_set_default_prec(50 * 1000L / 301L);
#endif
#ifdef TEST_MPREAL
   mpfr::mpreal::set_default_prec(50 * 1000L / 301L);
#endif

   bessel_tests_1();
   bessel_tests_2();
   bessel_tests_3();

   //
   // Then 100 digits:
   //
   std::cout << "Testing Bessel Functions at 100 digits....." << std::endl;
#if defined(TEST_MPFR) || defined(TEST_MPFR_CLASS)
   mpfr_set_default_prec(100 * 1000L / 301L);
#endif
   bessel_tests_4();
   bessel_tests_5();
   bessel_tests_6();
}
