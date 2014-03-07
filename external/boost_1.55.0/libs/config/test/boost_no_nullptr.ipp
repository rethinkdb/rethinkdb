//  (C) Copyright Beman Dawes 2009

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for more information.

//  MACRO:         BOOST_NO_CXX11_NULLPTR
//  TITLE:         C++0x nullptr feature unavailable
//  DESCRIPTION:   The compiler does not support the C++0x nullptr feature

namespace boost_no_cxx11_nullptr {

void quiet_warning(const int*){}

int test()
{
  int * p = nullptr;
  quiet_warning(p);
  return 0;
}

}
