//  (C) Copyright Beman Dawes 2009

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for more information.

//  MACRO:         BOOST_NO_CXX11_HDR_UNORDERED_MAP
//  TITLE:         C++0x header <unordered_map> unavailable
//  DESCRIPTION:   The standard library does not supply C++0x header <unordered_map>

#include <unordered_map>

namespace boost_no_cxx11_hdr_unordered_map {

int test()
{
  std::unordered_map<int, long> s1;
  std::unordered_multimap<int, long> s2;
  return s1.empty() && s2.empty() ? 0 : 1;
}

}
