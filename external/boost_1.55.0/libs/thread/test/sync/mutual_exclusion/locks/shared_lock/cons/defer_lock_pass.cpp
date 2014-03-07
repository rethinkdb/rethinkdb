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

// shared_lock(mutex_type& m, adopt_lock_t);

#include <boost/thread/lock_types.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/detail/lightweight_test.hpp>

int main()
{
  boost::shared_mutex m;
  m.lock();
  boost::shared_lock<boost::shared_mutex> lk(m, boost::defer_lock);
  BOOST_TEST(lk.mutex() == &m);
  BOOST_TEST(lk.owns_lock() == false);

  return boost::report_errors();
}

