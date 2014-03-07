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

// explicit operator bool() const;

#include <boost/thread/lock_types.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/detail/lightweight_test.hpp>

boost::mutex m;

int main()
{
  {
    boost::unique_lock<boost::mutex> lk0;
    BOOST_TEST(bool(lk0) == false);
    boost::unique_lock<boost::mutex> lk1(m);
    BOOST_TEST(bool(lk1) == true);
    lk1.unlock();
    BOOST_TEST(bool(lk1) == false);
  }

  {
    boost::unique_lock<boost::mutex> lk0;
    if (lk0) BOOST_TEST(false);
    boost::unique_lock<boost::mutex> lk1(m);
    if (!lk1) BOOST_TEST(false);
    lk1.unlock();
    if (lk1) BOOST_TEST(false);
  }
  return boost::report_errors();
}

