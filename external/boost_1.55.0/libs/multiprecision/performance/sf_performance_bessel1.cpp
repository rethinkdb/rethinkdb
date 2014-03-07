///////////////////////////////////////////////////////////////
//  Copyright 2011 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

#include "sf_performance.hpp"

void bessel_tests_1()
{
#ifdef TEST_MPFR
#if MPFR_VERSION<MPFR_VERSION_NUM(3,0,0)
   time_proc("mpfr_float_50", test_bessel<boost::multiprecision::mpfr_float_50>, 1);
   time_proc("mpfr_float_50 (no expression templates)", test_bessel<number<mpfr_float_backend<50>, et_off> >, 1);
   time_proc("static_mpfr_float_50", test_bessel<number<mpfr_float_backend<50, allocate_stack>, et_on> >, 1);
   time_proc("static_mpfr_float_50 (no expression templates)", test_bessel<number<mpfr_float_backend<50, allocate_stack>, et_off> >, 1);
#else
   time_proc("mpfr_float_50", test_bessel<boost::multiprecision::mpfr_float_50>, mpfr_buildopt_tls_p() ? 3 : 1);
   time_proc("mpfr_float_50 (no expression templates", test_bessel<number<mpfr_float_backend<50>, et_off> >, mpfr_buildopt_tls_p() ? 3 : 1);
   time_proc("static_mpfr_float_50", test_bessel<number<mpfr_float_backend<50, allocate_stack>, et_on> >, mpfr_buildopt_tls_p() ? 3 : 1);
   time_proc("static_mpfr_float_50 (no expression templates)", test_bessel<number<mpfr_float_backend<50, allocate_stack>, et_off> >, mpfr_buildopt_tls_p() ? 3 : 1);
#endif
#endif
}
