//  (C) Copyright Beman Dawes 2009

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for more information.

//  MACRO:         BOOST_NO_CXX11_HDR_FUTURE
//  TITLE:         C++0x header <future> unavailable
//  DESCRIPTION:   The standard library does not supply C++0x header <future>

#include <future>

namespace boost_no_cxx11_hdr_future {

int test()
{
  using std::is_error_code_enum;
  using std::make_error_code;
  using std::make_error_condition;
  using std::future_category;
  using std::future_error;
  using std::promise;
  using std::promise;
  using std::promise;
  using std::future;
  using std::shared_future;
  using std::packaged_task; // undefined
  using std::async;
  return 0;
}

}
