#ifndef BOOST_ATOMIC_DETAIL_GENERIC_CAS_HPP
#define BOOST_ATOMIC_DETAIL_GENERIC_CAS_HPP

//  Copyright (c) 2009 Helge Bahmann
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <cstddef>
#include <boost/cstdint.hpp>
#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/base.hpp>
#include <boost/atomic/detail/builder.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

/* fallback implementation for various compilation targets;
this is *not* efficient, particularly because all operations
are fully fenced (full memory barriers before and after
each operation) */

#if defined(__GNUC__)
    namespace boost { namespace atomics { namespace detail {
    inline int32_t
    fenced_compare_exchange_strong_32(volatile int32_t *ptr, int32_t expected, int32_t desired)
    {
        return __sync_val_compare_and_swap_4(ptr, expected, desired);
    }
    #define BOOST_ATOMIC_HAVE_CAS32 1

    #if defined(__amd64__) || defined(__i686__)
    inline int64_t
    fenced_compare_exchange_strong_64(int64_t *ptr, int64_t expected, int64_t desired)
    {
        return __sync_val_compare_and_swap_8(ptr, expected, desired);
    }
    #define BOOST_ATOMIC_HAVE_CAS64 1
    #endif
    }}}

#elif defined(__ICL) || defined(_MSC_VER)

    #if defined(_MSC_VER)
    #include <Windows.h>
    #include <intrin.h>
    #endif

    namespace boost { namespace atomics { namespace detail {
    inline int32_t
    fenced_compare_exchange_strong(int32_t *ptr, int32_t expected, int32_t desired)
    {
        return _InterlockedCompareExchange(reinterpret_cast<volatile long*>(ptr), desired, expected);
    }
    #define BOOST_ATOMIC_HAVE_CAS32 1
    #if defined(_WIN64)
    inline int64_t
    fenced_compare_exchange_strong(int64_t *ptr, int64_t expected, int64_t desired)
    {
        return _InterlockedCompareExchange64(ptr, desired, expected);
    }
    #define BOOST_ATOMIC_HAVE_CAS64 1
    #endif
    }}}

#elif (defined(__ICC) || defined(__ECC))
    namespace boost { namespace atomics { namespace detail {
    inline int32_t
    fenced_compare_exchange_strong_32(int32_t *ptr, int32_t expected, int32_t desired)
    {
        return _InterlockedCompareExchange((void*)ptr, desired, expected);
    }
    #define BOOST_ATOMIC_HAVE_CAS32 1
    #if defined(__x86_64)
    inline int64_t
    fenced_compare_exchange_strong(int64_t *ptr, int64_t expected, int64_t desired)
    {
        return cas64<int>(ptr, expected, desired);
    }
    #define BOOST_ATOMIC_HAVE_CAS64 1
    #elif defined(__ECC)    //IA-64 version
    inline int64_t
    fenced_compare_exchange_strong(int64_t *ptr, int64_t expected, int64_t desired)
    {
        return _InterlockedCompareExchange64((void*)ptr, desired, expected);
    }
    #define BOOST_ATOMIC_HAVE_CAS64 1
    #endif
    }}}

#elif (defined(__SUNPRO_CC) && defined(__sparc))
    #include <sys/atomic.h>
    namespace boost { namespace atomics { namespace detail {
    inline int32_t
    fenced_compare_exchange_strong_32(int32_t *ptr, int32_t expected, int32_t desired)
    {
        return atomic_cas_32((volatile unsigned int*)ptr, expected, desired);
    }
    #define BOOST_ATOMIC_HAVE_CAS32 1

    /* FIXME: check for 64 bit mode */
    inline int64_t
    fenced_compare_exchange_strong_64(int64_t *ptr, int64_t expected, int64_t desired)
    {
        return atomic_cas_64((volatile unsigned long long*)ptr, expected, desired);
    }
    #define BOOST_ATOMIC_HAVE_CAS64 1
    }}}
#endif


namespace boost {
namespace atomics {
namespace detail {

#ifdef BOOST_ATOMIC_HAVE_CAS32
template<typename T>
class atomic_generic_cas32
{
private:
    typedef atomic_generic_cas32 this_type;
public:
    explicit atomic_generic_cas32(T v) : i((int32_t)v) {}
    atomic_generic_cas32() {}
    T load(memory_order order=memory_order_seq_cst) const volatile
    {
        T expected=(T)i;
        do { } while(!const_cast<this_type *>(this)->compare_exchange_weak(expected, expected, order, memory_order_relaxed));
        return expected;
    }
    void store(T v, memory_order order=memory_order_seq_cst) volatile
    {
        exchange(v);
    }
    bool compare_exchange_strong(
        T &expected,
        T desired,
        memory_order success_order,
        memory_order failure_order) volatile
    {
        T found;
        found=(T)fenced_compare_exchange_strong_32(&i, (int32_t)expected, (int32_t)desired);
        bool success=(found==expected);
        expected=found;
        return success;
    }
    bool compare_exchange_weak(
        T &expected,
        T desired,
        memory_order success_order,
        memory_order failure_order) volatile
    {
        return compare_exchange_strong(expected, desired, success_order, failure_order);
    }
    T exchange(T r, memory_order order=memory_order_seq_cst) volatile
    {
        T expected=(T)i;
        do { } while(!compare_exchange_weak(expected, r, order, memory_order_relaxed));
        return expected;
    }

    bool is_lock_free(void) const volatile {return true;}
    typedef T integral_type;
private:
    mutable int32_t i;
};

template<typename T>
class platform_atomic_integral<T, 4> :
    public build_atomic_from_exchange<atomic_generic_cas32<T> >
{
public:
    typedef build_atomic_from_exchange<atomic_generic_cas32<T> > super;
    explicit platform_atomic_integral(T v) : super(v) {}
    platform_atomic_integral(void) {}
};

template<typename T>
class platform_atomic_integral<T, 1> :
   public build_atomic_from_larger_type<atomic_generic_cas32<int32_t>, T>
{
public:
    typedef build_atomic_from_larger_type<atomic_generic_cas32<int32_t>, T> super;

    explicit platform_atomic_integral(T v) : super(v) {}
    platform_atomic_integral(void) {}
};

template<typename T>
class platform_atomic_integral<T, 2> :
    public build_atomic_from_larger_type<atomic_generic_cas32<int32_t>, T>
{
public:
    typedef build_atomic_from_larger_type<atomic_generic_cas32<int32_t>, T> super;

    explicit platform_atomic_integral(T v) : super(v) {}
    platform_atomic_integral(void) {}
};
#endif

} } }

#endif
