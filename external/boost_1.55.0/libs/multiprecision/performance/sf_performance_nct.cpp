///////////////////////////////////////////////////////////////
//  Copyright 2011 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

#include "sf_performance.hpp"

void nct_tests_1();
void nct_tests_2();
void nct_tests_3();
void nct_tests_4();
void nct_tests_5();
void nct_tests_6();

void nct_tests()
{
   //
   // 50 digits first:
   //
   std::cout << "Testing Non-Central T at 50 digits....." << std::endl;
#ifdef TEST_MPFR_CLASS
   mpfr_set_default_prec(50 * 1000L / 301L);
#endif

   nct_tests_1();
   nct_tests_2();
   nct_tests_3();

   //
   // Then 100 digits:
   //
   std::cout << "Testing Non-Central T at 100 digits....." << std::endl;
#ifdef TEST_MPFR_CLASS
   mpfr_set_default_prec(100 * 1000L / 301L);
#endif
   nct_tests_4();
   nct_tests_5();
   nct_tests_6();
}
