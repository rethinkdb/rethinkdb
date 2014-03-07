//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_HAS_SIGACTION
//  TITLE:         sigaction
//  DESCRIPTION:   The platform supports POSIX standard API sigaction.

#include <signal.h>


namespace boost_has_sigaction{

void f()
{
    // this is never called, it just has to compile:
    struct sigaction* sa1 = 0 ;
    struct sigaction* sa2 = 0 ;
    int res = sigaction(0, sa1, sa2);
    (void) &res;
}

int test()
{
   return 0;
}

}





