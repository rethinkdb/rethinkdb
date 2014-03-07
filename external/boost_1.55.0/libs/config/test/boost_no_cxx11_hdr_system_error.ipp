//  (C) Copyright Beman Dawes 2009

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for more information.

//  MACRO:         BOOST_NO_CXX11_HDR_SYSTEM_ERROR
//  TITLE:         C++0x header <system_error> unavailable
//  DESCRIPTION:   The standard library does not supply C++0x header <system_error>

#include <system_error>

namespace boost_no_cxx11_hdr_system_error {

int test()
{
  using std::error_category;
  using std::error_code;
  using std::error_condition;
  using std::system_error;
  return 0;
}

}
