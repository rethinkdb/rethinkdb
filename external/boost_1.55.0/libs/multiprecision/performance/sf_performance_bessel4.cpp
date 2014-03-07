///////////////////////////////////////////////////////////////
//  Copyright 2011 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

#include "sf_performance.hpp"

void bessel_tests_4()
{
#ifdef TEST_MPFR
   time_proc("mpfr_float_100", test_bessel<mpfr_float_100>);
   time_proc("mpfr_float_100 (no expression templates", test_bessel<number<mpfr_float_backend<100>, et_off> >);
   time_proc("static_mpfr_float_100", test_bessel<static_mpfr_float_100>);
#endif
}
