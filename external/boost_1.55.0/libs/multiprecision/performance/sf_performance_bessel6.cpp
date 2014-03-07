///////////////////////////////////////////////////////////////
//  Copyright 2011 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

#include "sf_performance.hpp"

void bessel_tests_6()
{
#ifdef TEST_CPP_DEC_FLOAT
   time_proc("cpp_dec_float_100", test_bessel<cpp_dec_float_100>);
#endif
#ifdef TEST_MPFR_CLASS
   time_proc("mpfr_class", test_bessel<mpfr_class>);
#endif
#ifdef TEST_MPREAL
   time_proc("mpfr::mpreal", test_bessel<mpfr::mpreal>);
#endif
}
