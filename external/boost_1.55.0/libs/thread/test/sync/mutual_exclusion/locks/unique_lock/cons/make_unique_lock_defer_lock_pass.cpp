// Copyright (C) 2012 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// <boost/thread/lock_factories.hpp>

// template <class Mutex> class unique_lock;
// unique_lock<Mutex> make_unique_lock(Mutex&, defer_lock_t);

// unique_lock(mutex_type& m, adopt_lock_t);

#define BOOST_THREAD_VERSION 4

#include <boost/thread/lock_factories.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/detail/lightweight_test.hpp>

int main()
{
  boost::mutex m;
#if ! defined(BOOST_NO_CXX11_AUTO_DECLARATIONS)
  auto
#else
  boost::unique_lock<boost::mutex>
#endif
  lk = boost::make_unique_lock(m, boost::defer_lock);
  BOOST_TEST(lk.mutex() == &m);
  BOOST_TEST(lk.owns_lock() == false);

  return boost::report_errors();
}
