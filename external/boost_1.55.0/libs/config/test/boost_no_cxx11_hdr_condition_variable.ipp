//  (C) Copyright Beman Dawes 2009

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for more information.

//  MACRO:         BOOST_NO_CXX11_HDR_CONDITION_VARIABLE
//  TITLE:         C++0x header <condition_variable> unavailable
//  DESCRIPTION:   The standard library does not supply C++0x header <condition_variable>

#include <condition_variable>

namespace boost_no_cxx11_hdr_condition_variable {

int test()
{
  using std::condition_variable;
  using std::condition_variable_any;
  return 0;
}

}
