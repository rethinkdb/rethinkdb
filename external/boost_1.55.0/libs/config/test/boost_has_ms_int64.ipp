//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_HAS_MS_INT64
//  TITLE:         __int64
//  DESCRIPTION:   The platform supports Microsoft style __int64.

#include <cstdlib>


namespace boost_has_ms_int64{

int test()
{
   __int64 lli = 0i64;
   unsigned __int64 ulli = 0ui64;
   (void)lli;
   (void)ulli;
   return 0;
}

}






