///////////////////////////////////////////////////////////////
//  Copyright 2011 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

#include "sf_performance.hpp"

void nct_tests_1()
{
#ifdef TEST_MPFR
   time_proc("mpfr_float_50", test_nct<mpfr_float_50>);
   time_proc("mpfr_float_50 (no expression templates", test_nct<number<mpfr_float_backend<50>, et_off> >);
   time_proc("static_mpfr_float_50", test_nct<static_mpfr_float_50>);
#endif
}
