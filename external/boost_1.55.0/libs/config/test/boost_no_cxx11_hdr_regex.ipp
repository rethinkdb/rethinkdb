//  (C) Copyright Beman Dawes 2009

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for more information.

//  MACRO:         BOOST_NO_CXX11_HDR_REGEX
//  TITLE:         C++0x header <regex> unavailable
//  DESCRIPTION:   The standard library does not supply C++0x header <regex>

#include <regex>

namespace boost_no_cxx11_hdr_regex {

int test()
{
  using std::regex;
  using std::wregex;
  return 0;
}

}
