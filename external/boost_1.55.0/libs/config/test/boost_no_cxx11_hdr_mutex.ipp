//  (C) Copyright Beman Dawes 2009

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for more information.

//  MACRO:         BOOST_NO_CXX11_HDR_MUTEX
//  TITLE:         C++0x header <mutex> unavailable
//  DESCRIPTION:   The standard library does not supply C++0x header <mutex>

#include <mutex>

namespace boost_no_cxx11_hdr_mutex {

int test()
{
  using std::mutex;
  using std::recursive_mutex;
  using std::timed_mutex;
  using std::recursive_timed_mutex;
  return 0;
}

}
