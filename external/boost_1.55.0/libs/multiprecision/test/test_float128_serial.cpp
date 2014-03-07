///////////////////////////////////////////////////////////////
//  Copyright 2013 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

//
// Compare arithmetic results using fixed_int to GMP results.
//

#ifdef _MSC_VER
#  define _SCL_SECURE_NO_WARNINGS
#endif

#include <boost/multiprecision/float128.hpp>
#include "test_float_serial.hpp"


int main()
{
   using namespace boost::multiprecision;
   test<float128>();
   return boost::report_errors();
}



