//  (C) Copyright Beman Dawes 2009

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for more information.

//  MACRO:         BOOST_NO_CXX11_HDR_TUPLE
//  TITLE:         C++0x header <tuple> unavailable
//  DESCRIPTION:   The standard library does not supply C++0x header <tuple>

#include <tuple>

namespace boost_no_cxx11_hdr_tuple {

int test()
{
  std::tuple<int, int, long> t(0, 1, 2);
  return 0;
}

}
