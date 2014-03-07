//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_CWCTYPE
//  TITLE:         <wctype.h> and <cwctype>
//  DESCRIPTION:   The Platform does not provide <wctype.h> and <cwctype>.

//
// Note that some platforms put these prototypes in the wrong headers,
// we have to include pretty well all the string headers on the chance that
// one of them will contain what we want!
//
#include <cwctype>
#include <wctype.h>
#include <cwchar>
#include <wchar.h>
#include <cctype>
#include <ctype.h>

namespace boost_no_cwctype{

int test()
{
   if(!(iswalpha(L'a') &&
        iswcntrl(L'\r') &&
        iswdigit(L'2') &&
        iswlower(L'a') &&
        iswpunct(L',') &&
        iswspace(L' ') &&
        iswupper(L'A') &&
        iswxdigit(L'A')
   )) return -1;
   if(L'a' != towlower(L'A')) return -1;
   return 0;
}

}






