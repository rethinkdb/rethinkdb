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

// <boost/thread/locks.hpp>

// template <class Mutex> class shared_lock;

// explicit operator bool() const;

#include <boost/thread/lock_types.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/detail/lightweight_test.hpp>

boost::shared_mutex m;

int main()
{
  boost::shared_lock < boost::shared_mutex > lk0;
  BOOST_TEST(bool(lk0) == false);
  boost::shared_lock < boost::shared_mutex > lk1(m);
  BOOST_TEST(bool(lk1) == true);
  lk1.unlock();
  BOOST_TEST(bool(lk1) == false);

  return boost::report_errors();
}

