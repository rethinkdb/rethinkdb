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

//   bool unlock();

#include <boost/thread/lock_types.hpp>
//#include <boost/thread/mutex.hpp>
#include <boost/detail/lightweight_test.hpp>


bool unlock_called = false;

struct mutex
{
  void lock()
  {
  }
  void unlock()
  {
    unlock_called = true;
  }
};

mutex m;

int main()
{
  boost::unique_lock<mutex> lk(m);
  lk.unlock();
  BOOST_TEST(unlock_called == true);
  BOOST_TEST(lk.owns_lock() == false);
  try
  {
    lk.unlock();
    BOOST_TEST(false);
  }
  catch (boost::system::system_error& e)
  {
    BOOST_TEST(e.code().value() == boost::system::errc::operation_not_permitted);
  }
  lk.release();
  try
  {
    lk.unlock();
    BOOST_TEST(false);
  }
  catch (boost::system::system_error& e)
  {
    BOOST_TEST(e.code().value() == boost::system::errc::operation_not_permitted);
  }

  return boost::report_errors();
}

