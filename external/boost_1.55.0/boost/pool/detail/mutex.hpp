// Copyright (C) 2000 Stephen Cleary
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org for updates, documentation, and revision history.

#ifndef BOOST_POOL_MUTEX_HPP
#define BOOST_POOL_MUTEX_HPP

#include <boost/config.hpp>  // for workarounds
#ifdef BOOST_HAS_THREADS
#include <boost/thread/mutex.hpp>
#endif

namespace boost{ namespace details{ namespace pool{

class null_mutex
{
  private:
    null_mutex(const null_mutex &);
    void operator=(const null_mutex &);

  public:
    null_mutex() { }

    static void lock() { }
    static void unlock() { }
};

#if !defined(BOOST_HAS_THREADS) || defined(BOOST_NO_MT) || defined(BOOST_POOL_NO_MT)
  typedef null_mutex default_mutex;
#else
  typedef boost::mutex default_mutex;
#endif

} // namespace pool
} // namespace details
} // namespace boost

#endif
