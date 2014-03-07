//  (C) Copyright Beman Dawes 2009

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for more information.

//  MACRO:         BOOST_NO_CXX11_HDR_FORWARD_LIST
//  TITLE:         C++0x header <forward_list> unavailable
//  DESCRIPTION:   The standard library does not supply C++0x header <forward_list>

#include <forward_list>

namespace boost_no_cxx11_hdr_forward_list {

int test()
{
  std::forward_list<int> l(2u, 2);
  return *l.begin() == 2 ? 0 : 1;
}

}
