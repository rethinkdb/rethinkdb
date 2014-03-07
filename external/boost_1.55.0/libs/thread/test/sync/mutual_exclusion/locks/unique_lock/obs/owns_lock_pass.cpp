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

// bool owns_lock() const;

#include <boost/thread/lock_types.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/detail/lightweight_test.hpp>


int main()
{
  boost::mutex m;

  boost::unique_lock<boost::mutex> lk0;
  BOOST_TEST(lk0.owns_lock() == false);
  boost::unique_lock<boost::mutex> lk1(m);
  BOOST_TEST(lk1.owns_lock() == true);
  lk1.unlock();
  BOOST_TEST(lk1.owns_lock() == false);


  return boost::report_errors();
}

