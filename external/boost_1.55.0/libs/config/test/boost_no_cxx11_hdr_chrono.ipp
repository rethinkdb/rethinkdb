//  (C) Copyright Beman Dawes 2009

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for more information.

//  MACRO:         BOOST_NO_CXX11_HDR_CHRONO
//  TITLE:         C++0x header <chrono> unavailable
//  DESCRIPTION:   The standard library does not supply C++0x header <chrono>

#include <chrono>

namespace boost_no_cxx11_hdr_chrono {

int test()
{
  using std::chrono::nanoseconds;
  using std::chrono::microseconds;
  using std::chrono::milliseconds;
  using std::chrono::seconds;
  using std::chrono::minutes;
  using std::chrono::hours;
  using std::chrono::system_clock;
  using std::chrono::steady_clock;
  using std::chrono::high_resolution_clock;
  return 0;
}

}
