///////////////////////////////////////////////////////////////////////////////
//  Copyright 2012 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifdef TEST_MPFR

#include <boost/multiprecision/mpfr.hpp>

using namespace boost::multiprecision;

int main()
{
   mpfr_float_100 f(2);
   mpfr_float_50 f2;
   f2 = f;
}

#else

#error "Nothing to test without GMP!"

#endif
