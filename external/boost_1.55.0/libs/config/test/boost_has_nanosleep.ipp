//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_HAS_NANOSLEEP
//  TITLE:         nanosleep
//  DESCRIPTION:   The platform supports POSIX API nanosleep.

#include <time.h>


namespace boost_has_nanosleep{

void f()
{
    // this is never called, it just has to compile:
    timespec ts = {0, 0};
    timespec rm;
    int res = nanosleep(&ts, &rm);
    (void) &res;
}

int test()
{
   return 0;
}

}





