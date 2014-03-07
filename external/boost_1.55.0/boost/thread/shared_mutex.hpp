#ifndef BOOST_THREAD_SHARED_MUTEX_HPP
#define BOOST_THREAD_SHARED_MUTEX_HPP

//  shared_mutex.hpp
//
//  (C) Copyright 2007 Anthony Williams
//  (C) Copyright 2011-2012 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread/detail/config.hpp>
#if defined(BOOST_THREAD_PLATFORM_WIN32)
#if defined(BOOST_THREAD_PROVIDES_GENERIC_SHARED_MUTEX_ON_WIN)
#include <boost/thread/pthread/shared_mutex.hpp>
#else
#include <boost/thread/win32/shared_mutex.hpp>
#endif
#elif defined(BOOST_THREAD_PLATFORM_PTHREAD)
//#include <boost/thread/v2/shared_mutex.hpp>
#include <boost/thread/pthread/shared_mutex.hpp>
#else
#error "Boost threads unavailable on this platform"
#endif

#include <boost/thread/lockable_traits.hpp>

namespace boost
{
  namespace sync
  {
#ifdef BOOST_THREAD_NO_AUTO_DETECT_MUTEX_TYPES
    template<>
    struct is_basic_lockable<shared_mutex>
    {
      BOOST_STATIC_CONSTANT(bool, value = true);
    };
    template<>
    struct is_lockable<shared_mutex>
    {
      BOOST_STATIC_CONSTANT(bool, value = true);
    };
#endif

  }
}

#endif
