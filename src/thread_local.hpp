// Copyright 2010-2012 RethinkDB, all rights reserved.

/// @file thread_local.hpp
/// @brief Thread-local storage utilities with inline-safe accessor functions
///
/// Provides abstractions for thread-local storage (TLS) with particular care
/// to ensure compiler optimizations don't break TLS access across coroutine switches.
///
/// @defgroup ThreadLocalStorage Thread-Local Storage
/// Safe thread-local variable access preventing compiler optimization issues
/// @{

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

/// @brief Thread-local storage with safe accessor functions
///
/// @details We must ensure that access to thread local storage (TLS) is only
/// performed from functions that cannot be inlined.
///
/// **The Problem:**
/// Consider this code:
/// @code
/// int before = TLS_get_x();
/// on_thread_t switcher(...);
/// int after = TLS_get_x();
/// @endcode
///
/// The `after` value should be from the new thread, and `before` from the old thread.
/// However, if the compiler inlines TLS_get_x(), it will generate pseudocode like:
/// @code
/// void *__tls_segment = register "%gs";  // Thread-local segment register
/// int *__addr_of_x = __tls_segment + __x_tls_offset;
/// int before = *__addr_of_x;
/// on_thread_t switcher(...);
/// void *__tls_segment = register "%gs";  // Should reload but doesn't!
/// int *__addr_of_x = __tls_segment + __x_tls_offset;
/// int after = *__addr_of_x;
/// @endcode
///
/// The compiler will see the identical `__tls_segment` expressions and optimize the
/// second one away, caching the old thread's value. Then `after` will have the wrong value.
///
/// **The Solution:**
/// We ensure TLS accessors are never inlined. This forces the compiler to reload the
/// TLS segment register (%gs on x86, %fs on others) every time we call an accessor.
///
/// **Note:**
/// Making variables volatile won't help here because we need the TLS segment *lookup*
/// to be volatile, not just the variable access.
///
/// @see arch/runtime/coroutines.hpp for thread switching mechanisms
/// @}

#ifndef THREAD_LOCAL_HPP_
#define THREAD_LOCAL_HPP_
 * cannot be inlined, and that it does not use on_thread_t.
 */

#ifndef THREADED_COROUTINES

#define DEFINE_TLS_REF_ACCESSORS(type, name)                            \
    NOINLINE type& TLS_get_ref_ ## name () {                            \
        return TLS_ ## name;                                            \
    }                                                                   \
                                                                        \
    template <class T>                                                  \
    NOINLINE void TLS_set_ ## name (T&& val) {                          \
        TLS_ ## name = std::forward<T>(val);                            \
    }

#define DEFINE_TLS_ACCESSORS(type, name)                                \
    DEFINE_TLS_REF_ACCESSORS(type, name)                                \
    NOINLINE type TLS_get_ ## name () {                                 \
        return TLS_get_ref_ ## name ();                                 \
    }

#define TLS(type, name)                                                 \
    static thread_local type TLS_ ## name;                              \
    DEFINE_TLS_ACCESSORS(type, name)

#define TLS_with_get_ref(type, name)                                    \
    static thread_local type TLS_ ## name;                              \
    DEFINE_TLS_REF_ACCESSORS(type, name)

#define TLS_with_init(type, name, initial)                              \
    static thread_local type TLS_ ## name(initial);                     \
    DEFINE_TLS_ACCESSORS(type, name)

#else  // THREADED_COROUTINES

#define DEFINE_TLS_REF_ACCESSORS(type, name)                            \
    type& TLS_get_ref_ ## name () {                                     \
        return TLS_ ## name[get_thread_id().threadnum].value;           \
    }                                                                   \
                                                                        \
    template <class T>                                                  \
    void TLS_set_ ## name(T&& val) {                                    \
        TLS_ ## name[get_thread_id().threadnum].value = std::forward<T>(val); \
    }

#define DEFINE_TLS_ACCESSORS(type, name)                                \
    DEFINE_TLS_REF_ACCESSORS(type, name)                                \
    type TLS_get_ ## name () {                                          \
        return TLS_get_ref_ ## name ();                                 \
    }

#define TLS(type, name)                                                 \
    static std::vector<cache_line_padded_t<type> >                      \
        TLS_ ## name(MAX_CORES);                                        \
    DEFINE_TLS_ACCESSORS(type, name)

#define TLS_with_get_ref(type, name)                                    \
    static std::vector<cache_line_padded_t<type> >                      \
        TLS_ ## name(MAX_CORES);                                        \
    DEFINE_TLS_REF_ACCESSORS(type, name)

#define TLS_with_init(type, name, initial)                              \
    static std::vector<cache_line_padded_t<type> >                      \
        TLS_ ## name(MAX_CORES, cache_line_padded_t<type>(initial));    \
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
