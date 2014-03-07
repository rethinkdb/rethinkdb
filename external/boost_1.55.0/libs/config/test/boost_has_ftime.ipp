//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_HAS_FTIME
//  TITLE:         The platform has FTIME.
//  DESCRIPTION:   The platform supports the Win32 API type FTIME.

#include <windows.h>


namespace boost_has_ftime{

void f(FILETIME)
{
    // this is never called, it just has to compile:
}

int test()
{
   return 0;
}

}





