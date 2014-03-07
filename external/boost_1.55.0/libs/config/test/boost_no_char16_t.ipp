
//  (C) Copyright Beman Dawes 2008

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for more information.

//  MACRO:         BOOST_NO_CXX11_CHAR16_T
//  TITLE:         C++0x char16_t unavailable
//  DESCRIPTION:   The compiler does not support C++0x char16_t

namespace boost_no_cxx11_char16_t {

//  Microsoft VC++ 2010, and possibly other compilers, provides
//  a typedef for char16_t rather than a new type. We want that
//  to be an error, so provide an overloaded function that will
//  be ambiguous if char16_t is just a typedef.
void f(const char16_t*){}
void f(const unsigned short*){}
void f(const unsigned int*){}
void f(const unsigned long*){}

int test()
{
  const char16_t p(0);
  f(&p);
  return 0;
}

}
