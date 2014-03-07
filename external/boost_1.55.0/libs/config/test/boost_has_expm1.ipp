//  (C) Copyright John Maddock 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_HAS_EXPM1
//  TITLE:         expm1
//  DESCRIPTION:   The std lib has a C99-conforming expm1 function.

#include <math.h>

namespace boost_has_expm1{

int test()
{
   double x = 0.5;
   x = ::expm1(x);
   (void)x;
   return 0;
}

}

