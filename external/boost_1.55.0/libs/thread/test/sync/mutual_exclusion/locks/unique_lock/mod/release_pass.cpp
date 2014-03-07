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

// template <class Mutex> class unique_lock;

// void Mutex* release();

#include <boost/thread/lock_types.hpp>
#include <boost/detail/lightweight_test.hpp>

struct mutex
{
  static int lock_count;
  static int unlock_count;
  void lock()
  {
    ++lock_count;
  }
  void unlock()
  {
    ++unlock_count;
  }
};

int mutex::lock_count = 0;
int mutex::unlock_count = 0;

mutex m;

int main()
{
  boost::unique_lock<mutex> lk(m);
  BOOST_TEST(lk.mutex() == &m);
  BOOST_TEST(lk.owns_lock() == true);
  BOOST_TEST(mutex::lock_count == 1);
  BOOST_TEST(mutex::unlock_count == 0);
  BOOST_TEST(lk.release() == &m);
  BOOST_TEST(lk.mutex() == 0);
  BOOST_TEST(lk.owns_lock() == false);
  BOOST_TEST(mutex::lock_count == 1);
  BOOST_TEST(mutex::unlock_count == 0);

  return boost::report_errors();
}

