//  Copyright (C) 2011 Tim Blechmann
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_LOCKFREE_DETAIL_ATOMIC_HPP
#define BOOST_LOCKFREE_DETAIL_ATOMIC_HPP

#include <boost/config.hpp>

// at this time, few compiles completely implement atomic<>
#define BOOST_LOCKFREE_NO_HDR_ATOMIC

// MSVC supports atomic<> from version 2012 onwards.
#if defined(BOOST_MSVC) && (BOOST_MSVC >= 1700)
#undef BOOST_LOCKFREE_NO_HDR_ATOMIC
#endif

// GCC supports atomic<> from version 4.8 onwards.
#if defined(__GNUC__)
# if defined(__GNUC_PATCHLEVEL__)
#  define BOOST_ATOMIC_GNUC_VERSION (__GNUC__ * 10000           \
                                     + __GNUC_MINOR__ * 100     \
                                     + __GNUC_PATCHLEVEL__)
# else
#  define BOOST_LOCKFREE_GNUC_VERSION (__GNUC__ * 10000         \
                                     + __GNUC_MINOR__ * 100)
# endif
#endif

#if (BOOST_LOCKFREE_GNUC_VERSION >= 40800) && (__cplusplus >= 201103L)
#undef BOOST_LOCKFREE_NO_HDR_ATOMIC
#endif

#undef BOOST_LOCKFREE_GNUC_VERSION

#if defined(BOOST_LOCKFREE_NO_HDR_ATOMIC)
#include <boost/atomic.hpp>
#else
#include <atomic>
#endif

namespace boost {
namespace lockfree {
namespace detail {

#if defined(BOOST_LOCKFREE_NO_HDR_ATOMIC)
using boost::atomic;
using boost::memory_order_acquire;
using boost::memory_order_consume;
using boost::memory_order_relaxed;
using boost::memory_order_release;
#else
using std::atomic;
using std::memory_order_acquire;
using std::memory_order_consume;
using std::memory_order_relaxed;
using std::memory_order_release;
#endif

}
using detail::atomic;
using detail::memory_order_acquire;
using detail::memory_order_consume;
using detail::memory_order_relaxed;
using detail::memory_order_release;

}}

#endif /* BOOST_LOCKFREE_DETAIL_ATOMIC_HPP */
