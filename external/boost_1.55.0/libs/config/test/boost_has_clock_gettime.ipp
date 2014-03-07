//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_HAS_CLOCK_GETTIME
//  TITLE:         clock_gettime
//  DESCRIPTION:   The platform supports POSIX standard API clock_gettime.

#include <time.h>


namespace boost_has_clock_gettime{

void f()
{
    // this is never called, it just has to compile:
    timespec tp;
    int res = clock_gettime(CLOCK_REALTIME, &tp);
    (void) &res;
}

int test()
{
   return 0;
}

}





