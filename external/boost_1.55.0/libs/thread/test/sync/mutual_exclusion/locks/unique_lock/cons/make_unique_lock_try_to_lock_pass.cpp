// Copyright (C) 2012 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// <boost/thread/locks.hpp>

// template <class Mutex> class unique_lock;
// unique_lock<Mutex> make_unique_lock(Mutex&, try_to_lock_t);

#define BOOST_THREAD_VERSION 4


#include <boost/thread/lock_factories.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/detail/lightweight_test.hpp>

boost::mutex m;

#if defined BOOST_THREAD_USES_CHRONO
typedef boost::chrono::system_clock Clock;
typedef Clock::time_point time_point;
typedef Clock::duration duration;
typedef boost::chrono::milliseconds ms;
typedef boost::chrono::nanoseconds ns;
#else
#endif

void f()
{
#if defined BOOST_THREAD_USES_CHRONO
  {
    time_point t0 = Clock::now();
#if ! defined(BOOST_NO_CXX11_AUTO_DECLARATIONS)
  auto
#else
  boost::unique_lock<boost::mutex>
#endif
    lk = boost::make_unique_lock(m, boost::try_to_lock);
    BOOST_TEST(lk.owns_lock() == false);
    time_point t1 = Clock::now();
    ns d = t1 - t0 - ms(250);
    // This test is spurious as it depends on the time the thread system switches the threads
    BOOST_TEST(d < ns(50000000)+ms(1000)); // within 50ms
  }
  {
    time_point t0 = Clock::now();
#if ! defined(BOOST_NO_CXX11_AUTO_DECLARATIONS)
  auto
#else
  boost::unique_lock<boost::mutex>
#endif
    lk = boost::make_unique_lock(m, boost::try_to_lock);
    BOOST_TEST(lk.owns_lock() == false);
    time_point t1 = Clock::now();
    ns d = t1 - t0 - ms(250);
    // This test is spurious as it depends on the time the thread system switches the threads
    BOOST_TEST(d < ns(50000000)+ms(1000)); // within 50ms
  }
  {
    time_point t0 = Clock::now();
#if ! defined(BOOST_NO_CXX11_AUTO_DECLARATIONS)
  auto
#else
  boost::unique_lock<boost::mutex>
#endif
    lk = boost::make_unique_lock(m, boost::try_to_lock);
    BOOST_TEST(lk.owns_lock() == false);
    time_point t1 = Clock::now();
    ns d = t1 - t0 - ms(250);
    // This test is spurious as it depends on the time the thread system switches the threads
    BOOST_TEST(d < ns(50000000)+ms(1000)); // within 50ms
  }
  {
    time_point t0 = Clock::now();
    while (true)
    {
#if ! defined(BOOST_NO_CXX11_AUTO_DECLARATIONS)
      auto
#else
      boost::unique_lock<boost::mutex>
#endif
      lk = boost::make_unique_lock(m, boost::try_to_lock);
      if (lk.owns_lock()) break;
    }
    time_point t1 = Clock::now();
    ns d = t1 - t0 - ms(250);
    // This test is spurious as it depends on the time the thread system switches the threads
    BOOST_TEST(d < ns(50000000)+ms(1000)); // within 50ms
  }
#else
//  time_point t0 = Clock::now();
//  {
//    boost::unique_lock<boost::mutex> lk(m, boost::try_to_lock);
//    BOOST_TEST(lk.owns_lock() == false);
//  }
//  {
//    boost::unique_lock<boost::mutex> lk(m, boost::try_to_lock);
//    BOOST_TEST(lk.owns_lock() == false);
//  }
//  {
//    boost::unique_lock<boost::mutex> lk(m, boost::try_to_lock);
//    BOOST_TEST(lk.owns_lock() == false);
//  }
  while (true)
  {
#if ! defined(BOOST_NO_CXX11_AUTO_DECLARATIONS)
  auto
#else
  boost::unique_lock<boost::mutex>
#endif
    lk = boost::make_unique_lock(m, boost::try_to_lock);
    if (lk.owns_lock()) break;
  }
  //time_point t1 = Clock::now();
  //ns d = t1 - t0 - ms(250);
  // This test is spurious as it depends on the time the thread system switches the threads
  //BOOST_TEST(d < ns(50000000)+ms(1000)); // within 50ms
#endif
}

int main()
{
  m.lock();
  boost::thread t(f);
#if defined BOOST_THREAD_USES_CHRONO
  boost::this_thread::sleep_for(ms(250));
#else
#endif
  m.unlock();
  t.join();

  return boost::report_errors();
}
