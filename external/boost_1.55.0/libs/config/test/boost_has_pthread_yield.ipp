//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_HAS_PTHREAD_YIELD
//  TITLE:         pthread_yield
//  DESCRIPTION:   The platform supports non standard API pthread_yield.

#include <pthread.h>


namespace boost_has_pthread_yield{

void f()
{
    // this is never called, it just has to compile:
    int res = pthread_yield();
}

int test()
{
   return 0;
}

}





