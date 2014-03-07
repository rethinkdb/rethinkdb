//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_CWCHAR
//  TITLE:         <wchar.h> and <cwchar>
//  DESCRIPTION:   The Platform does not provide <wchar.h> and <cwchar>.

#include <cwchar>
#include <wchar.h>

namespace boost_no_cwchar{

int test()
{
   wchar_t c1[2] = { 0 };
   wchar_t c2[2] = { 0 };
   if(wcscmp(c1,c2) || wcslen(c1)) return -1;
   wcscpy(c1,c2);
   wcsxfrm(c1,c2,0);
   return 0;
}

}






