#ifndef BOOST_ATOMIC_DETAIL_CAS64STRONG_PTR_HPP
#define BOOST_ATOMIC_DETAIL_CAS64STRONG_PTR_HPP

//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  Copyright (c) 2011 Helge Bahmann
//  Copyright (c) 2013 Tim Blechmann

// Build 64-bit atomic operation on pointers from platform_cmpxchg64_strong
// primitive. It is assumed that 64-bit loads/stores are not
// atomic, so they are implemented through platform_load64/platform_store64.
//
// The reason for extracting pointer specializations to a separate header is
// that 64-bit CAS is available on some 32-bit platforms (notably, x86).
// On these platforms there is no need for 64-bit pointer specializations,
// since they will never be used.

#include <string.h>
#include <cstddef>
#include <boost/cstdint.hpp>
#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/base.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

/* pointer types */

template<bool Sign>
class base_atomic<void *, void *, 8, Sign>
{
private:
    typedef base_atomic this_type;
    typedef void * value_type;
    typedef std::ptrdiff_t difference_type;

protected:
    typedef value_type value_arg_type;

public:
    BOOST_DEFAULTED_FUNCTION(base_atomic(void), {})
    BOOST_CONSTEXPR explicit base_atomic(value_type v) BOOST_NOEXCEPT : v_(v) {}

    void
    store(value_type v, memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        platform_fence_before_store(order);
        platform_store64(v, &v_);
        platform_fence_after_store(order);
    }

    value_type
    load(memory_order order = memory_order_seq_cst) const volatile BOOST_NOEXCEPT
    {
        value_type v = platform_load64(&v_);
        platform_fence_after_load(order);
        return v;
    }

    value_type
    exchange(value_type v, memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        value_type original = load(memory_order_relaxed);
        do {
        } while (!compare_exchange_weak(original, v, order, memory_order_relaxed));
        return original;
    }

    bool
    compare_exchange_weak(
        value_type & expected,
        value_type desired,
        memory_order success_order,
        memory_order failure_order) volatile BOOST_NOEXCEPT
    {
        return compare_exchange_strong(expected, desired, success_order, failure_order);
    }

    bool
    compare_exchange_strong(
        value_type & expected,
        value_type desired,
        memory_order success_order,
        memory_order failure_order) volatile BOOST_NOEXCEPT
    {
        platform_fence_before(success_order);

        bool success = platform_cmpxchg64_strong(expected, desired, &v_);

        if (success) {
            platform_fence_after(success_order);
        } else {
            platform_fence_after(failure_order);
        }

        return success;
    }

    value_type
    fetch_add(difference_type v, memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        value_type original = load(memory_order_relaxed);
        do {
        } while (!compare_exchange_weak(original, (char*)original + v, order, memory_order_relaxed));
        return original;
    }

    value_type
    fetch_sub(difference_type v, memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        value_type original = load(memory_order_relaxed);
        do {
        } while (!compare_exchange_weak(original, (char*)original - v, order, memory_order_relaxed));
        return original;
    }

    bool
    is_lock_free(void) const volatile BOOST_NOEXCEPT
    {
        return true;
    }

    BOOST_ATOMIC_DECLARE_VOID_POINTER_OPERATORS

    BOOST_DELETED_FUNCTION(base_atomic(base_atomic const&))
    BOOST_DELETED_FUNCTION(base_atomic& operator=(base_atomic const&))

private:
    value_type v_;
};

template<typename T, bool Sign>
class base_atomic<T *, void *, 8, Sign>
{
private:
    typedef base_atomic this_type;
    typedef T * value_type;
    typedef std::ptrdiff_t difference_type;

protected:
    typedef value_type value_arg_type;

public:
    BOOST_DEFAULTED_FUNCTION(base_atomic(void), {})
    BOOST_CONSTEXPR explicit base_atomic(value_type v) BOOST_NOEXCEPT : v_(v) {}

    void
    store(value_type v, memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        platform_fence_before_store(order);
        platform_store64(v, &v_);
        platform_fence_after_store(order);
    }

    value_type
    load(memory_order order = memory_order_seq_cst) const volatile BOOST_NOEXCEPT
    {
        value_type v = platform_load64(&v_);
        platform_fence_after_load(order);
        return v;
    }

    value_type
    exchange(value_type v, memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        value_type original = load(memory_order_relaxed);
        do {
        } while (!compare_exchange_weak(original, v, order, memory_order_relaxed));
        return original;
    }

    bool
    compare_exchange_weak(
        value_type & expected,
        value_type desired,
        memory_order success_order,
        memory_order failure_order) volatile BOOST_NOEXCEPT
    {
        return compare_exchange_strong(expected, desired, success_order, failure_order);
    }

    bool
    compare_exchange_strong(
        value_type & expected,
        value_type desired,
        memory_order success_order,
        memory_order failure_order) volatile BOOST_NOEXCEPT
    {
        platform_fence_before(success_order);

        bool success = platform_cmpxchg64_strong(expected, desired, &v_);

        if (success) {
            platform_fence_after(success_order);
        } else {
            platform_fence_after(failure_order);
        }

        return success;
    }

    value_type
    fetch_add(difference_type v, memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        value_type original = load(memory_order_relaxed);
        do {
        } while (!compare_exchange_weak(original, original + v, order, memory_order_relaxed));
        return original;
    }

    value_type
    fetch_sub(difference_type v, memory_order order = memory_order_seq_cst) volatile BOOST_NOEXCEPT
    {
        value_type original = load(memory_order_relaxed);
        do {
        } while (!compare_exchange_weak(original, original - v, order, memory_order_relaxed));
        return original;
    }

    bool
    is_lock_free(void) const volatile BOOST_NOEXCEPT
    {
        return true;
    }

    BOOST_ATOMIC_DECLARE_POINTER_OPERATORS

    BOOST_DELETED_FUNCTION(base_atomic(base_atomic const&))
    BOOST_DELETED_FUNCTION(base_atomic& operator=(base_atomic const&))

private:
    value_type v_;
};

}
}
}

#endif
