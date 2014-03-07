//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_HAS_PTHREAD_DELAY_NP
//  TITLE:         pthread_delay_np
//  DESCRIPTION:   The platform supports non-standard pthread_delay_np API.

#include <pthread.h>
#include <time.h>


namespace boost_has_pthread_delay_np{

void f()
{
    // this is never called, it just has to compile:
    timespec ts;
    int res = pthread_delay_np(&ts);
}

int test()
{
   return 0;
}

}





