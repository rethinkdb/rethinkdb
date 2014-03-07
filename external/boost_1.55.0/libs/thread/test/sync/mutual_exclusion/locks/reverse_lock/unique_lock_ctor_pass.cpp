// Copyright (C) 2012 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// <boost/thread/locks.hpp>

// template <class Mutex> class unlock_guard;

// unlock_guard(unlock_guard const&) = delete;

#include <boost/thread/reverse_lock.hpp>
#include <boost/thread/lock_types.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/detail/lightweight_test.hpp>


int main()
{
  {
    boost::mutex m;
    boost::unique_lock<boost::mutex> lk(m);
    BOOST_TEST(lk.owns_lock());
    BOOST_TEST(lk.mutex()==&m);

    {
      boost::reverse_lock<boost::unique_lock<boost::mutex> > lg(lk);
      BOOST_TEST(!lk.owns_lock());
      BOOST_TEST(lk.mutex()==0);
    }
    BOOST_TEST(lk.owns_lock());
    BOOST_TEST(lk.mutex()==&m);
  }

  {
    boost::mutex m;
    boost::unique_lock<boost::mutex> lk(m, boost::defer_lock);
    BOOST_TEST(! lk.owns_lock());
    BOOST_TEST(lk.mutex()==&m);
    {
      boost::reverse_lock<boost::unique_lock<boost::mutex> > lg(lk);
      BOOST_TEST(!lk.owns_lock());
      BOOST_TEST(lk.mutex()==0);
    }
    BOOST_TEST(lk.owns_lock());
    BOOST_TEST(lk.mutex()==&m);
  }


  return boost::report_errors();
}
