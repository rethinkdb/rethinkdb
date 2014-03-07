///////////////////////////////////////////////////////////////
//  Copyright 2011 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

#include "sf_performance.hpp"

void nct_tests_2()
{
#ifdef TEST_MPF
   time_proc("mpf_float_50", test_nct<mpf_float_50>);
   time_proc("mpf_float_50 (no expression templates", test_nct<number<gmp_float<50>, et_off> >);
#endif
}
