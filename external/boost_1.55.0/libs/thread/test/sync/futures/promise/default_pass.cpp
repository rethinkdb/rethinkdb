//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// Copyright (C) 2011 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// <boost/thread/future.hpp>

// class promise<R>

// promise();

#define BOOST_THREAD_VERSION 3

#include <boost/thread/future.hpp>
#include <boost/detail/lightweight_test.hpp>

int main()
{

  {
      boost::promise<int> p;
      boost::future<int> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
      BOOST_TEST(f.valid());
  }
  {
      boost::promise<int&> p;
      boost::future<int&> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
      BOOST_TEST(f.valid());
  }
  {
      boost::promise<void> p;
      boost::future<void> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
      BOOST_TEST(f.valid());
  }

  return boost::report_errors();
}

