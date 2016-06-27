// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef THREAD_LOCAL_HPP_
#define THREAD_LOCAL_HPP_

#ifdef THREADED_COROUTINES
#include <vector>
#include "config/args.hpp"
// For get_thread_id()
#include "arch/runtime/coroutines.hpp"
#endif

#include "arch/compiler.hpp"
#include "errors.hpp"
#include "concurrency/cache_line_padded.hpp"
#include "utils.hpp"

/*
 * We have to make sure that access to thread local storage (TLS) is only performed
 * from functions that cannot be inlined.
 *
 * Consider the following code:
 *     int before = TLS_get_x();
 *     on_thread_t switcher(...);
 *     int after = TLS_get_x();
 *
 * `after` should be the value of `x` on the new thread, and `before` the one on the
 * old thread.
 *
 * Now if the compiler is allowed to inline TLS_get_x(), it will internally generate
 * something like this (pseudocode):
 *     void *__tls_segment = register "%gs";
 *     int *__addr_of_x = __tls_segment + __x_tls_offset;
 *     int before = *__addr_of_x;
 *     on_thread_t switcher(...);
 *     __tls_segment = register "%gs";
 *     _addr_of_x = __tls_segment + __x_tls_offset;
 *     int after = *__addr_of_x;
 *
 * So far so good. The value of %gs will have changed after on_thread_t, and
 * `after` is going to have the value of x on the new thread.
 * Note that I'm using %gs here as the register for the TLS memory region.
 * Other architectures will use different registers (e.g. %fs).
 * Unfortunately, the compiler does not know that %gs can change in the middle
 * of this function, and for GCC as of version 4.8, there doesn't seem to be a
 * way of telling it that it can.
 * So the compiler will look for common subexpressions and optimize them away,
 * making the generated code look more like this:
 *     void *__tls_segment = register "%gs";
 *     int *__addr_of_x = __tls_segment + __x_tls_offset;
 *     int before = *__addr_of_x;
 *     on_thread_t switcher(...);
 *     int after = *__addr_of_x;
 *
 * Now `after` will have the value of x on the *old* thread. This is obviously
 * not correct.
 *
 * Also note that making x volatile is not going to solve this, because
 * we would need the compiler-generated __tls_segment to be volatile.
 *
 * So in essence, we must make sure that any function which accesses TLS directly
 * cannot be inlined, and that it does not use on_thread_t.
 */

#ifndef THREADED_COROUTINES

#define DEFINE_TLS_ACCESSORS(type, name)                                \
    NOINLINE type TLS_get_ ## name () {                                 \
        return TLS_ ## name;                                            \
    }                                                                   \
                                                                        \
    template <class T>                                                  \
    NOINLINE void TLS_set_ ## name (T&& val) {                          \
        TLS_ ## name = std::forward<T>(val);                            \
    }

#define TLS(type, name)                                                 \
    static THREAD_LOCAL type TLS_ ## name;                              \
    DEFINE_TLS_ACCESSORS(type, name)

#define TLS_with_init(type, name, initial)                              \
    static THREAD_LOCAL type TLS_ ## name(initial);                     \
    DEFINE_TLS_ACCESSORS(type, name)

#else  // THREADED_COROUTINES

#define DEFINE_TLS_ACCESSORS(type, name)                                \
    type TLS_get_ ## name () {                                          \
        return TLS_ ## name[get_thread_id().threadnum].value;           \
    }                                                                   \
                                                                        \
    template <class T>                                                  \
    void TLS_set_ ## name(T&& val) {                                    \
        TLS_ ## name[get_thread_id().threadnum].value = std::forward<T>(val); \
    }

#define TLS(type, name)                                                 \
    static std::vector<cache_line_padded_t<type> >                      \
        TLS_ ## name(MAX_THREADS);                                      \
    DEFINE_TLS_ACCESSORS(type, name)

#define TLS_with_init(type, name, initial)                              \
    static std::vector<cache_line_padded_t<type> >                      \
        TLS_ ## name(MAX_THREADS, cache_line_padded_t<type>(initial));  \
    DEFINE_TLS_ACCESSORS(type, name)

#endif  // THREADED_COROUTINES

#define GLIBCXX_4_8 20130322

#if defined(_LIBCPP_TYPE_TRAITS) || defined(_MSC_VER) || __GLIBCXX__ >= GLIBCXX_4_8
// libc++ with type traights support, visual studio and libstdc++ >= 4.8
using std::is_trivially_destructible;
#else
#define is_trivially_destructible std::has_trivial_destructor
#endif

#endif /* THREAD_LOCAL_HPP_ */
