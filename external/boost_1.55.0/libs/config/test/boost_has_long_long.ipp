//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_HAS_LONG_LONG
//  TITLE:         long long
//  DESCRIPTION:   The platform supports long long.

#include <cstdlib>


namespace boost_has_long_long{

int test()
{
#ifdef __GNUC__
   __extension__ long long lli = 0LL;
   __extension__ unsigned long long ulli = 0uLL;
#else
   long long lli = 0LL;
   unsigned long long ulli = 0uLL;
#endif
   (void)&lli;
   (void)&ulli;
   return 0;
}

}





