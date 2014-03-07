//  (C) Copyright John Maddock 2012. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_HAS_INT128
//  TITLE:         __int128
//  DESCRIPTION:   The platform supports __int128.

#include <cstdlib>


namespace boost_has_int128{

int test()
{
#ifdef __GNUC__
   __extension__ __int128 lli = 0;
   __extension__ unsigned __int128 ulli = 0u;
#else
   __int128 lli = 0;
   unsigned __int128 ulli = 0u;
#endif
   (void)&lli;
   (void)&ulli;
   return 0;
}

}





