//  (C) Copyright John Maddock 2011. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_HAS_GETSYSTEMTIMEASFILETIME
//  TITLE:         GetSystemTimeAsFileTime
//  DESCRIPTION:   The platform supports Win32 API GetSystemTimeAsFileTime.

#include <windows.h>


namespace boost_has_getsystemtimeasfiletime{

void f()
{
    // this is never called, it just has to compile:
   FILETIME ft;
   GetSystemTimeAsFileTime(&ft);
}

int test()
{
   return 0;
}

}

