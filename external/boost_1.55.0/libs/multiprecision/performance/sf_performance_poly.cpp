///////////////////////////////////////////////////////////////
//  Copyright 2011 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

#include "sf_performance.hpp"

void poly_tests()
{
   //
   // 50 digits first:
   //
   std::cout << "Testing Polynomial Evaluation at 50 digits....." << std::endl;
#if defined(TEST_MPFR) || defined(TEST_MPFR_CLASS)
   mpfr_set_default_prec(50 * 1000L / 301L);
#endif
#ifdef TEST_MPREAL
   mpfr::mpreal::set_default_prec(50 * 1000L / 301L);
#endif
#ifdef TEST_MPFR
   time_proc("mpfr_float_50", test_polynomial<mpfr_float_50>);
   time_proc("mpfr_float_50 (no expression templates", test_polynomial<number<mpfr_float_backend<50>, et_off> >);
   time_proc("static_mpfr_float_50", test_polynomial<static_mpfr_float_50>);
#endif
#ifdef TEST_MPF
   time_proc("mpf_float_50", test_polynomial<mpf_float_50>);
   time_proc("mpf_float_50 (no expression templates", test_polynomial<number<gmp_float<50>, et_off> >);
#endif
#ifdef TEST_CPP_DEC_FLOAT
   time_proc("cpp_dec_float_50", test_polynomial<cpp_dec_float_50>);
#endif
#ifdef TEST_MPFR_CLASS
   time_proc("mpfr_class", test_polynomial<mpfr_class>);
#endif
#ifdef TEST_MPREAL
   time_proc("mpfr::mpreal", test_polynomial<mpfr::mpreal>);
#endif
   //
   // Then 100 digits:
   //
   std::cout << "Testing Polynomial Evaluation at 100 digits....." << std::endl;
#ifdef TEST_MPFR_CLASS
   mpfr_set_default_prec(100 * 1000L / 301L);
#endif
#ifdef TEST_MPREAL
   mpfr::mpreal::set_default_prec(100 * 1000L / 301L);
#endif
#ifdef TEST_MPFR
   time_proc("mpfr_float_100", test_polynomial<mpfr_float_100>);
   time_proc("mpfr_float_100 (no expression templates", test_polynomial<number<mpfr_float_backend<100>, et_off> >);
   time_proc("static_mpfr_float_100", test_polynomial<static_mpfr_float_100>);
#endif
#ifdef TEST_MPF
   time_proc("mpf_float_100", test_polynomial<mpf_float_100>);
   time_proc("mpf_float_100 (no expression templates", test_polynomial<number<gmp_float<100>, et_off> >);
#endif
#ifdef TEST_CPP_DEC_FLOAT
   time_proc("cpp_dec_float_100", test_polynomial<cpp_dec_float_100>);
#endif
#ifdef TEST_MPFR_CLASS
   time_proc("mpfr_class", test_polynomial<mpfr_class>);
#endif
#ifdef TEST_MPREAL
   time_proc("mpfr::mpreal", test_polynomial<mpfr::mpreal>);
#endif
}
