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

// template <class Mutex>
//   void swap(shared_lock<Mutex>& x, shared_lock<Mutex>& y);

#include <boost/thread/lock_types.hpp>
#include <boost/detail/lightweight_test.hpp>

struct shared_mutex
{
  void lock_shared()
  {
  }
  void unlock_shared()
  {
  }
};

shared_mutex m;

int main()
{
  boost::shared_lock<shared_mutex> lk1(m);
  boost::shared_lock<shared_mutex> lk2;
  swap(lk1, lk2);
  BOOST_TEST(lk1.mutex() == 0);
  BOOST_TEST(lk1.owns_lock() == false);
  BOOST_TEST(lk2.mutex() == &m);
  BOOST_TEST(lk2.owns_lock() == true);

  return boost::report_errors();
}

