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

// class future<R>

// future();

#define BOOST_THREAD_VERSION 3

#include <boost/thread/future.hpp>
#include <boost/detail/lightweight_test.hpp>

int main()
{

  {
      boost::future<int> f;
      BOOST_TEST(!f.valid());
  }
  {
      boost::future<int&> f;
      BOOST_TEST(!f.valid());
  }
  {
      boost::future<void> f;
      BOOST_TEST(!f.valid());
  }


  return boost::report_errors();
}

