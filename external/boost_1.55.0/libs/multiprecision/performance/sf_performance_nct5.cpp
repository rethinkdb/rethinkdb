///////////////////////////////////////////////////////////////
//  Copyright 2011 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

#include "sf_performance.hpp"

void nct_tests_5()
{
#ifdef TEST_MPF
   time_proc("mpf_float_100", test_nct<mpf_float_100>);
   time_proc("mpf_float_100 (no expression templates", test_nct<number<gmp_float<100>, et_off> >);
#endif
}
