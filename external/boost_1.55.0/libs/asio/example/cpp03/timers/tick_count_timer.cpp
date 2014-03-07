//
// tick_count_timer.cpp
// ~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/asio.hpp>
#include <ctime>
#include <iostream>

#if defined(WIN32)
# include <windows.h>
#else
# error This example is for Windows only!
#endif

struct tick_count_traits
{
  // The time type. This type has no constructor that takes a DWORD to ensure
  // that the timer can only be used with relative times.
  class time_type
  {
  public:
    time_type() : ticks_(0) {}
  private:
    friend struct tick_count_traits;
    DWORD ticks_;
  };

  // The duration type. According to the TimeTraits requirements, the duration
  // must be a signed type. This means we can't handle durations larger than
  // 2^31.
  class duration_type
  {
  public:
    duration_type() : ticks_(0) {}
    duration_type(LONG ticks) : ticks_(ticks) {}
  private:
    friend struct tick_count_traits;
    LONG ticks_;
  };

  // Get the current time.
  static time_type now()
  {
    time_type result;
    result.ticks_ = ::GetTickCount();
    return result;
  }

  // Add a duration to a time.
  static time_type add(const time_type& t, const duration_type& d)
  {
    time_type result;
    result.ticks_ = t.ticks_ + d.ticks_;
    return result;
  }

  // Subtract one time from another.
  static duration_type subtract(const time_type& t1, const time_type& t2)
  {
    // DWORD tick count values can wrap (see less_than() below). We'll convert
    // to a duration by taking the absolute difference and adding the sign
    // based on which is the "lesser" absolute time.
    return duration_type(less_than(t1, t2)
        ? -static_cast<LONG>(t2.ticks_ - t1.ticks_)
        : static_cast<LONG>(t1.ticks_ - t2.ticks_));
  }

  // Test whether one time is less than another.
  static bool less_than(const time_type& t1, const time_type& t2)
  {
    // DWORD tick count values wrap periodically, so we'll use a heuristic that
    // says that if subtracting t1 from t2 yields a value smaller than 2^31,
    // then t1 is probably less than t2. This means that we can't handle
    // durations larger than 2^31, which shouldn't be a problem in practice.
    return (t2.ticks_ - t1.ticks_) < static_cast<DWORD>(1 << 31);
  }

  // Convert to POSIX duration type.
  static boost::posix_time::time_duration to_posix_duration(
      const duration_type& d)
  {
    return boost::posix_time::milliseconds(d.ticks_);
  }
};

typedef boost::asio::basic_deadline_timer<
    DWORD, tick_count_traits> tick_count_timer;

void handle_timeout(const boost::system::error_code&)
{
  std::cout << "handle_timeout\n";
}

int main()
{
  try
  {
    boost::asio::io_service io_service;

    tick_count_timer timer(io_service, 5000);
    std::cout << "Starting synchronous wait\n";
    timer.wait();
    std::cout << "Finished synchronous wait\n";

    timer.expires_from_now(5000);
    std::cout << "Starting asynchronous wait\n";
    timer.async_wait(&handle_timeout);
    io_service.run();
    std::cout << "Finished asynchronous wait\n";
  }
  catch (std::exception& e)
  {
    std::cout << "Exception: " << e.what() << "\n";
  }

  return 0;
}
