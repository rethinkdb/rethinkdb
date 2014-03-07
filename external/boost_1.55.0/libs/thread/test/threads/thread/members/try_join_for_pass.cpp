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

//        template <class Rep, class Period>
//        bool try_join_for(const chrono::duration<Rep, Period>& rel_time);

#define BOOST_THREAD_VESRION 3
#include <boost/thread/thread_only.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <new>
#include <cstdlib>
#include <cassert>
#include <iostream>
#include <boost/detail/lightweight_test.hpp>

#if defined BOOST_THREAD_USES_CHRONO

class G
{
  int alive_;
public:
  static bool op_run;

  G() :
    alive_(1)
  {
  }
  G(const G& g) :
    alive_(g.alive_)
  {
  }
  ~G()
  {
    alive_ = 0;
  }

  void operator()()
  {
    BOOST_TEST(alive_ == 1);
    op_run = true;
  }
};

bool G::op_run = false;

boost::thread* resource_deadlock_would_occur_th=0;
boost::mutex resource_deadlock_would_occur_mtx;
void resource_deadlock_would_occur_tester()
{
  try
  {
    boost::unique_lock<boost::mutex> lk(resource_deadlock_would_occur_mtx);
    resource_deadlock_would_occur_th->try_join_for(boost::chrono::milliseconds(50));
    BOOST_TEST(false);
  }
  catch (boost::system::system_error& e)
  {
    BOOST_TEST(e.code().value() == boost::system::errc::resource_deadlock_would_occur);
  }
  catch (...)
  {
    BOOST_TEST(false&&"exception thrown");
  }
}

void th_100_ms()
{
  boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
}

int main()
{
  {
    boost::thread t0( (G()));
    BOOST_TEST(t0.joinable());
    BOOST_TEST(t0.try_join_for(boost::chrono::milliseconds(150)));
    BOOST_TEST(!t0.joinable());
  }
  {
    boost::thread t0( (th_100_ms));
    BOOST_TEST(!t0.try_join_for(boost::chrono::milliseconds(50)));
    t0.join();
  }

  {
    boost::unique_lock<boost::mutex> lk(resource_deadlock_would_occur_mtx);
    boost::thread t0( resource_deadlock_would_occur_tester );
    resource_deadlock_would_occur_th = &t0;
    BOOST_TEST(t0.joinable());
    lk.unlock();
    boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
    boost::unique_lock<boost::mutex> lk2(resource_deadlock_would_occur_mtx);
    t0.join();
    BOOST_TEST(!t0.joinable());
  }

  {
    boost::thread t0( (G()));
    t0.detach();
    try
    {
      t0.try_join_for(boost::chrono::milliseconds(50));
      BOOST_TEST(false);
    }
    catch (boost::system::system_error& e)
    {
      BOOST_TEST(e.code().value() == boost::system::errc::invalid_argument);
    }
  }
  {
    boost::thread t0( (G()));
    BOOST_TEST(t0.joinable());
    t0.join();
    try
    {
      t0.try_join_for(boost::chrono::milliseconds(50));
      BOOST_TEST(false);
    }
    catch (boost::system::system_error& e)
    {
      BOOST_TEST(e.code().value() == boost::system::errc::invalid_argument);
    }

  }
  {
    boost::thread t0( (G()));
    BOOST_TEST(t0.joinable());
    t0.try_join_for(boost::chrono::milliseconds(50));
    try
    {
      t0.join();
      BOOST_TEST(false);
    }
    catch (boost::system::system_error& e)
    {
      BOOST_TEST(e.code().value() == boost::system::errc::invalid_argument);
    }

  }

  return boost::report_errors();
}

#else
#error "Test not applicable: BOOST_THREAD_USES_CHRONO not defined for this platform as not supported"
#endif
