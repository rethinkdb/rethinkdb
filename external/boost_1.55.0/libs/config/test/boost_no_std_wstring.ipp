//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_STD_WSTRING
//  TITLE:         std::wstring
//  DESCRIPTION:   The standard library lacks std::wstring.

#include <string>

namespace boost_no_std_wstring{

int test()
{
   std::wstring s;
   if(*s.c_str() || (s.begin() != s.end()) || s.size() || (sizeof(std::wstring::value_type) != sizeof(wchar_t))) return -1;
   return 0;
}

}





