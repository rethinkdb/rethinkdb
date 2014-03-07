//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_SWPRINTF
//  TITLE:         swprintf
//  DESCRIPTION:   The platform does not have a conforming version of swprintf.

#include <wchar.h>
#include <stdio.h>


namespace boost_no_swprintf{

int test()
{
   wchar_t buf[10];
   swprintf(buf, 10, L"%d", 10);
   return 0;
}

}






