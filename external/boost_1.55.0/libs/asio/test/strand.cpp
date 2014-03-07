//
// strand.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// Disable autolinking for unit tests.
#if !defined(BOOST_ALL_NO_LIB)
#define BOOST_ALL_NO_LIB 1
#endif // !defined(BOOST_ALL_NO_LIB)

// Test that header file is self-contained.
#include <boost/asio/strand.hpp>

#include <sstream>
#include <boost/asio/io_service.hpp>
#include <boost/asio/detail/thread.hpp>
#include "unit_test.hpp"

#if defined(BOOST_ASIO_HAS_BOOST_DATE_TIME)
# include <boost/asio/deadline_timer.hpp>
#else // defined(BOOST_ASIO_HAS_BOOST_DATE_TIME)
# include <boost/asio/steady_timer.hpp>
#endif // defined(BOOST_ASIO_HAS_BOOST_DATE_TIME)

#if defined(BOOST_ASIO_HAS_BOOST_BIND)
# include <boost/bind.hpp>
#else // defined(BOOST_ASIO_HAS_BOOST_BIND)
# include <functional>
#endif // defined(BOOST_ASIO_HAS_BOOST_BIND)

using namespace boost::asio;

#if defined(BOOST_ASIO_HAS_BOOST_BIND)
namespace bindns = boost;
#else // defined(BOOST_ASIO_HAS_BOOST_BIND)
namespace bindns = std;
#endif

#if defined(BOOST_ASIO_HAS_BOOST_DATE_TIME)
typedef deadline_timer timer;
namespace chronons = boost::posix_time;
#elif defined(BOOST_ASIO_HAS_STD_CHRONO)
typedef steady_timer timer;
namespace chronons = std::chrono;
#elif defined(BOOST_ASIO_HAS_BOOST_CHRONO)
typedef steady_timer timer;
namespace chronons = boost::chrono;
#endif // defined(BOOST_ASIO_HAS_BOOST_DATE_TIME)

void increment(int* count)
{
  ++(*count);
}

void increment_without_lock(strand* s, int* count)
{
  BOOST_ASIO_CHECK(!s->running_in_this_thread());

  int original_count = *count;

  s->dispatch(bindns::bind(increment, count));

  // No other functions are currently executing through the locking dispatcher,
  // so the previous call to dispatch should have successfully nested.
  BOOST_ASIO_CHECK(*count == original_count + 1);
}

void increment_with_lock(strand* s, int* count)
{
  BOOST_ASIO_CHECK(s->running_in_this_thread());

  int original_count = *count;

  s->dispatch(bindns::bind(increment, count));

  // The current function already holds the strand's lock, so the
  // previous call to dispatch should have successfully nested.
  BOOST_ASIO_CHECK(*count == original_count + 1);
}

void sleep_increment(io_service* ios, int* count)
{
  timer t(*ios, chronons::seconds(2));
  t.wait();

  ++(*count);
}

void start_sleep_increments(io_service* ios, strand* s, int* count)
{
  // Give all threads a chance to start.
  timer t(*ios, chronons::seconds(2));
  t.wait();

  // Start three increments.
  s->post(bindns::bind(sleep_increment, ios, count));
  s->post(bindns::bind(sleep_increment, ios, count));
  s->post(bindns::bind(sleep_increment, ios, count));
}

void throw_exception()
{
  throw 1;
}

void io_service_run(io_service* ios)
{
  ios->run();
}

void strand_test()
{
  io_service ios;
  strand s(ios);
  int count = 0;

  ios.post(bindns::bind(increment_without_lock, &s, &count));

  // No handlers can be called until run() is called.
  BOOST_ASIO_CHECK(count == 0);

  ios.run();

  // The run() call will not return until all work has finished.
  BOOST_ASIO_CHECK(count == 1);

  count = 0;
  ios.reset();
  s.post(bindns::bind(increment_with_lock, &s, &count));

  // No handlers can be called until run() is called.
  BOOST_ASIO_CHECK(count == 0);

  ios.run();

  // The run() call will not return until all work has finished.
  BOOST_ASIO_CHECK(count == 1);

  count = 0;
  ios.reset();
  ios.post(bindns::bind(start_sleep_increments, &ios, &s, &count));
  boost::asio::detail::thread thread1(bindns::bind(io_service_run, &ios));
  boost::asio::detail::thread thread2(bindns::bind(io_service_run, &ios));

  // Check all events run one after another even though there are two threads.
  timer timer1(ios, chronons::seconds(3));
  timer1.wait();
  BOOST_ASIO_CHECK(count == 0);
  timer1.expires_at(timer1.expires_at() + chronons::seconds(2));
  timer1.wait();
  BOOST_ASIO_CHECK(count == 1);
  timer1.expires_at(timer1.expires_at() + chronons::seconds(2));
  timer1.wait();
  BOOST_ASIO_CHECK(count == 2);

  thread1.join();
  thread2.join();

  // The run() calls will not return until all work has finished.
  BOOST_ASIO_CHECK(count == 3);

  count = 0;
  int exception_count = 0;
  ios.reset();
  s.post(throw_exception);
  s.post(bindns::bind(increment, &count));
  s.post(bindns::bind(increment, &count));
  s.post(throw_exception);
  s.post(bindns::bind(increment, &count));

  // No handlers can be called until run() is called.
  BOOST_ASIO_CHECK(count == 0);
  BOOST_ASIO_CHECK(exception_count == 0);

  for (;;)
  {
    try
    {
      ios.run();
      break;
    }
    catch (int)
    {
      ++exception_count;
    }
  }

  // The run() calls will not return until all work has finished.
  BOOST_ASIO_CHECK(count == 3);
  BOOST_ASIO_CHECK(exception_count == 2);

  count = 0;
  ios.reset();

  // Check for clean shutdown when handlers posted through an orphaned strand
  // are abandoned.
  {
    strand s2(ios);
    s2.post(bindns::bind(increment, &count));
    s2.post(bindns::bind(increment, &count));
    s2.post(bindns::bind(increment, &count));
  }

  // No handlers can be called until run() is called.
  BOOST_ASIO_CHECK(count == 0);
}

BOOST_ASIO_TEST_SUITE
(
  "strand",
  BOOST_ASIO_TEST_CASE(strand_test)
)
