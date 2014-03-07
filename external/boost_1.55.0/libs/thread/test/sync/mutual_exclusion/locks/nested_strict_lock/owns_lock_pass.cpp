// Copyright (C) 2012 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// <boost/thread/locks.hpp>

// template <class Mutex> class nested_strict_lock;

// bool owns_lock(Mutex *) const;

#include <boost/thread/lock_types.hpp>
#include <boost/thread/strict_lock.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/detail/lightweight_test.hpp>

int main()
{
  boost::mutex m;
  boost::mutex m2;
  {
    boost::unique_lock<boost::mutex> lk(m);
    {
      boost::nested_strict_lock<boost::unique_lock<boost::mutex> > nlk(lk);
      BOOST_TEST(nlk.owns_lock(&m) == true);
      BOOST_TEST(!nlk.owns_lock(&m2) == true);
    }
    BOOST_TEST(lk.owns_lock() == true && lk.mutex()==&m);
  }
  {
    m.lock();
    boost::unique_lock<boost::mutex> lk(m, boost::adopt_lock);
    {
      boost::nested_strict_lock<boost::unique_lock<boost::mutex> > nlk(lk);
      BOOST_TEST(nlk.owns_lock(&m) == true);
      BOOST_TEST(!nlk.owns_lock(&m2) == true);
    }
    BOOST_TEST(lk.owns_lock() == true && lk.mutex()==&m);
  }
  {
    boost::unique_lock<boost::mutex> lk(m, boost::defer_lock);
    {
      boost::nested_strict_lock<boost::unique_lock<boost::mutex> > nlk(lk);
      BOOST_TEST(nlk.owns_lock(&m) == true);
      BOOST_TEST(!nlk.owns_lock(&m2) == true);
    }
    BOOST_TEST(lk.owns_lock() == true && lk.mutex()==&m);
  }
  {
    boost::unique_lock<boost::mutex> lk(m, boost::try_to_lock);
    {
      boost::nested_strict_lock<boost::unique_lock<boost::mutex> > nlk(lk);
      BOOST_TEST(nlk.owns_lock(&m) == true);
      BOOST_TEST(!nlk.owns_lock(&m2) == true);
    }
    BOOST_TEST(lk.owns_lock() == true && lk.mutex()==&m);
  }

  return boost::report_errors();
}
