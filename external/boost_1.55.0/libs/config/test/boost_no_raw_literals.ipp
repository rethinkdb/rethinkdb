//  (C) Copyright Beman Dawes 2008

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for more information.

//  MACRO:         BOOST_NO_CXX11_RAW_LITERALS
//  TITLE:         C++0x raw string literals unavailable
//  DESCRIPTION:   The compiler does not support C++0x raw string literals

namespace boost_no_cxx11_raw_literals {

void quiet_warning(const char*){}
void quiet_warning(const wchar_t*){}

int test()
{
  const char* s = R"(abc)";
  quiet_warning(s);
  const wchar_t* ws = LR"(abc)";
  quiet_warning(ws);
  return 0;
}

}
