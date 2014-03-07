///////////////////////////////////////////////////////////////
//  Copyright 2011 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

#include "sf_performance.hpp"

void bessel_tests_2()
{
#ifdef TEST_MPF
   time_proc("mpf_float_50", test_bessel<mpf_float_50>, 3);
   time_proc("mpf_float_50 (no expression templates", test_bessel<number<gmp_float<50>, et_off> >, 3);
#endif
#ifdef TEST_CPP_DEC_FLOAT
   time_proc("cpp_dec_float_50", test_bessel<cpp_dec_float_50>, 3);
#endif
}
