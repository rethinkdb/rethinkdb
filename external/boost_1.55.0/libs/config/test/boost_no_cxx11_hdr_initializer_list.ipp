//  (C) Copyright Beman Dawes 2009

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for more information.

//  MACRO:         BOOST_NO_CXX11_HDR_INITIALIZER_LIST
//  TITLE:         C++0x header <initializer_list> unavailable
//  DESCRIPTION:   The standard library does not supply C++0x header <initializer_list>

#include <initializer_list>

namespace boost_no_cxx11_hdr_initializer_list {

void foo(const std::initializer_list<const char*>&)
{
}

int test()
{
  foo( { "a", "b", "c" } );
  return 0;
}

}
