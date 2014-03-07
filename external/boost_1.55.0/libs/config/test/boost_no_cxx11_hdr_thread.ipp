//  (C) Copyright Beman Dawes 2009

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for more information.

//  MACRO:         BOOST_NO_CXX11_HDR_THREAD
//  TITLE:         C++0x header <thread> unavailable
//  DESCRIPTION:   The standard library does not supply C++0x header <thread>

#include <thread>

namespace boost_no_cxx11_hdr_thread {

int test()
{
  using std::thread;
  using std::this_thread::get_id;
  using std::this_thread::yield;
  using std::this_thread::sleep_until;
  using std::this_thread::sleep_for;
  return 0;
}

}
