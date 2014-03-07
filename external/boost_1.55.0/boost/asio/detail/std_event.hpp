//
// detail/std_event.hpp
// ~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_STD_EVENT_HPP
#define BOOST_ASIO_DETAIL_STD_EVENT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if defined(BOOST_ASIO_HAS_STD_MUTEX_AND_CONDVAR)

#include <chrono>
#include <condition_variable>
#include <boost/asio/detail/assert.hpp>
#include <boost/asio/detail/noncopyable.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

class std_event
  : private noncopyable
{
public:
  // Constructor.
  std_event()
    : signalled_(false)
  {
  }

  // Destructor.
  ~std_event()
  {
  }

  // Signal the event.
  template <typename Lock>
  void signal(Lock& lock)
  {
    BOOST_ASIO_ASSERT(lock.locked());
    (void)lock;
    signalled_ = true;
    cond_.notify_one();
  }

  // Signal the event and unlock the mutex.
  template <typename Lock>
  void signal_and_unlock(Lock& lock)
  {
    BOOST_ASIO_ASSERT(lock.locked());
    signalled_ = true;
    lock.unlock();
    cond_.notify_one();
  }

  // Reset the event.
  template <typename Lock>
  void clear(Lock& lock)
  {
    BOOST_ASIO_ASSERT(lock.locked());
    (void)lock;
    signalled_ = false;
  }

  // Wait for the event to become signalled.
  template <typename Lock>
  void wait(Lock& lock)
  {
    BOOST_ASIO_ASSERT(lock.locked());
    unique_lock_adapter u_lock(lock);
    while (!signalled_)
      cond_.wait(u_lock.unique_lock_);
  }

  // Timed wait for the event to become signalled.
  template <typename Lock>
  bool wait_for_usec(Lock& lock, long usec)
  {
    BOOST_ASIO_ASSERT(lock.locked());
    unique_lock_adapter u_lock(lock);
    if (!signalled_)
      cond_.wait_for(u_lock.unique_lock_, std::chrono::microseconds(usec));
    return signalled_;
  }

private:
  // Helper class to temporarily adapt a scoped_lock into a unique_lock so that
  // it can be passed to std::condition_variable::wait().
  struct unique_lock_adapter
  {
    template <typename Lock>
    explicit unique_lock_adapter(Lock& lock)
      : unique_lock_(lock.mutex().mutex_, std::adopt_lock)
    {
    }

    ~unique_lock_adapter()
    {
      unique_lock_.release();
    }

    std::unique_lock<std::mutex> unique_lock_;
  };

  std::condition_variable cond_;
  bool signalled_;
};

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // defined(BOOST_ASIO_HAS_STD_MUTEX_AND_CONDVAR)

#endif // BOOST_ASIO_DETAIL_STD_EVENT_HPP
