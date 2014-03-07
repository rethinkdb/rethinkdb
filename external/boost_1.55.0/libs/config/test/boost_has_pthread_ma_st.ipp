//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE
//  TITLE:         pthread_mutexattr_settype
//  DESCRIPTION:   The platform supports POSIX API pthread_mutexattr_settype.

#include <pthread.h>


namespace boost_has_pthread_mutexattr_settype{

void f()
{
    // this is never called, it just has to compile:
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    int type = 0;
    pthread_mutexattr_settype(&attr, type);
}

int test()
{
   return 0;
}

}






