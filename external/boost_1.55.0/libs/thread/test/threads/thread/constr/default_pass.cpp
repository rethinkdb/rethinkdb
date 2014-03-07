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

// <boost/thread/thread.hpp>

// class thread

// thread();

#include <boost/thread/thread_only.hpp>
#include <cassert>
#include <boost/detail/lightweight_test.hpp>

int main()
{
  boost::thread t;
  BOOST_TEST(t.get_id() == boost::thread::id());
  return boost::report_errors();
}
