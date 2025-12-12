// Copyright 2010-2012 RethinkDB, all rights reserved.

/**
 * @file thread_local.hpp
 * @brief Thread-local storage (TLS) management macros.
 *
 * Provides portable macros for declaring and accessing thread-local variables
 * with proper compiler inlining guarantees. Supports both standard C++17 TLS
 * and coroutine-based thread simulation.
 *
 * Important: TLS access functions are always non-inlined to ensure correct
 * behavior across thread switches and context changes.
 *
 * @section tls_design Design Rationale
 *
 * Thread-local storage access requires careful handling to prevent compiler
 * optimizations from breaking correctness. When a thread context switch occurs,
 * the hardware register pointing to the TLS segment changes. If compiler inlines
 * TLS access functions, it may cache the TLS segment address and reuse it after
 * a thread switch, returning stale values from the old thread.
 */

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

/**
 * @brief Design notes for thread-local storage implementation.
 *
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

/**
 * @defgroup TLSMacros Thread-Local Storage Macros
 * @brief Non-inlined TLS access for standard C++17 thread_local
 */

/**
 * @ingroup TLSMacros
 * @def DEFINE_TLS_REF_ACCESSORS(type, name)
 * @brief Define getter and setter functions for non-inlined TLS reference access.
 *
 * Creates `TLS_get_ref_<name>()` and `TLS_set_<name>(value)` functions.
 * The reference accessor allows direct modification of the TLS variable.
 *
 * @param type The type of the thread-local variable.
 * @param name The name (identifier) of the TLS variable.
 *
 * Example:
 * @code
 * DEFINE_TLS_REF_ACCESSORS(int, counter);
 * TLS_get_ref_counter() += 1;  // Increment the thread-local counter
 * @endcode
 */
#define DEFINE_TLS_REF_ACCESSORS(type, name)                            \
    NOINLINE type& TLS_get_ref_ ## name () {                            \
        return TLS_ ## name;                                            \
    }                                                                   \
                                                                        \
    template <class T>                                                  \
    NOINLINE void TLS_set_ ## name (T&& val) {                          \
        TLS_ ## name = std::forward<T>(val);                            \
    }

/**
 * @ingroup TLSMacros
 * @def DEFINE_TLS_ACCESSORS(type, name)
 * @brief Define getter and setter functions for value-based TLS access.
 *
 * Creates `TLS_get_ref_<name>()`, `TLS_set_<name>(value)`, and
 * `TLS_get_<name>()` functions. The value accessor returns a copy.
 *
 * @param type The type of the thread-local variable.
 * @param name The name (identifier) of the TLS variable.
 *
 * Example:
 * @code
 * DEFINE_TLS_ACCESSORS(std::string, thread_name);
 * std::string name = TLS_get_thread_name();  // Returns copy
 * @endcode
 */
#define DEFINE_TLS_ACCESSORS(type, name)                                \
    DEFINE_TLS_REF_ACCESSORS(type, name)                                \
    NOINLINE type TLS_get_ ## name () {                                 \
        return TLS_get_ref_ ## name ();                                 \
    }

/**
 * @ingroup TLSMacros
 * @def TLS(type, name)
 * @brief Declare a thread-local variable with accessors.
 *
 * Creates a thread_local static variable and generates both
 * reference and value accessor functions (DEFINE_TLS_ACCESSORS).
 *
 * @param type The type of the thread-local variable.
 * @param name The name of the variable (used for accessor functions).
 *
 * Example:
 * @code
 * TLS(random_engine_t, rng);  // Creates TLS_rng with TLS_get_ref_rng() and TLS_get_rng()
 * @endcode
 */
#define TLS(type, name)                                                 \
    static thread_local type TLS_ ## name;                              \
    DEFINE_TLS_ACCESSORS(type, name)

/**
 * @ingroup TLSMacros
 * @def TLS_with_get_ref(type, name)
 * @brief Declare a thread-local variable with only reference accessors.
 *
 * Like TLS() but only generates reference and setter accessors,
 * without the value-returning accessor.
 *
 * @param type The type of the thread-local variable.
 * @param name The name of the variable.
 */
#define TLS_with_get_ref(type, name)                                    \
    static thread_local type TLS_ ## name;                              \
    DEFINE_TLS_REF_ACCESSORS(type, name)

/**
 * @ingroup TLSMacros
 * @def TLS_with_init(type, name, initial)
 * @brief Declare a thread-local variable with an initializer.
 *
 * Creates a thread_local static variable initialized to a value,
 * with full accessor functions.
 *
 * @param type The type of the thread-local variable.
 * @param name The name of the variable.
 * @param initial The initial value for each thread's instance.
 *
 * Example:
 * @code
 * TLS_with_init(int, thread_id, -1);  // Initialize to -1
 * @endcode
 */
#define TLS_with_init(type, name, initial)                              \
    static thread_local type TLS_ ## name(initial);                     \
    DEFINE_TLS_ACCESSORS(type, name)

#else  // THREADED_COROUTINES

/**
 * @ingroup TLSMacros
 * @def DEFINE_TLS_REF_ACCESSORS(type, name)
 * @brief Define getter/setter for coroutine-based TLS (uses vector storage).
 *
 * When THREADED_COROUTINES is enabled, TLS is implemented via per-core
 * vectors instead of thread_local. Thread ID is obtained via get_thread_id().
 *
 * @param type The type of the thread-local variable.
 * @param name The name (identifier) of the TLS variable.
 */
#define DEFINE_TLS_REF_ACCESSORS(type, name)                            \
    type& TLS_get_ref_ ## name () {                                     \
        return TLS_ ## name[get_thread_id().threadnum].value;           \
    }                                                                   \
                                                                        \
    template <class T>                                                  \
    void TLS_set_ ## name(T&& val) {                                    \
        TLS_ ## name[get_thread_id().threadnum].value = std::forward<T>(val); \
    }

/**
 * @ingroup TLSMacros
 * @def DEFINE_TLS_ACCESSORS(type, name)
 * @brief Define getter/setter for coroutine-based TLS (returns value/ref).
 *
 * Similar to non-coroutine version but accesses per-core vector storage.
 *
 * @param type The type of the thread-local variable.
 * @param name The name of the variable.
 */
#define DEFINE_TLS_ACCESSORS(type, name)                                \
    DEFINE_TLS_REF_ACCESSORS(type, name)                                \
    type TLS_get_ ## name () {                                          \
        return TLS_get_ref_ ## name ();                                 \
    }

/**
 * @ingroup TLSMacros
 * @def TLS(type, name)
 * @brief Declare a coroutine-based TLS variable with accessors.
 *
 * Creates a vector of cache-line-padded values (one per core) instead
 * of using thread_local. Accessors use get_thread_id() for lookup.
 *
 * @param type The type of the thread-local variable.
 * @param name The name of the variable.
 */
#define TLS(type, name)                                                 \
    static std::vector<cache_line_padded_t<type> >                      \
        TLS_ ## name(MAX_CORES);                                        \
    DEFINE_TLS_ACCESSORS(type, name)

/**
 * @ingroup TLSMacros
 * @def TLS_with_get_ref(type, name)
 * @brief Declare a coroutine-based TLS variable with only ref accessors.
 *
 * @param type The type of the thread-local variable.
 * @param name The name of the variable.
 */
#define TLS_with_get_ref(type, name)                                    \
    static std::vector<cache_line_padded_t<type> >                      \
        TLS_ ## name(MAX_CORES);                                        \
    DEFINE_TLS_REF_ACCESSORS(type, name)

/**
 * @ingroup TLSMacros
 * @def TLS_with_init(type, name, initial)
 * @brief Declare a coroutine-based TLS variable with initializer.
 *
 * Creates a vector of cache-line-padded values, each initialized to the
 * provided value.
 *
 * @param type The type of the thread-local variable.
 * @param name The name of the variable.
 * @param initial The initial value for each core's instance.
 */
#define TLS_with_init(type, name, initial)                              \
    static std::vector<cache_line_padded_t<type> >                      \
        TLS_ ## name(MAX_CORES, cache_line_padded_t<type>(initial));    \
    DEFINE_TLS_ACCESSORS(type, name)

#endif  // THREADED_COROUTINES

/**
 * @defgroup TypeTraits Type Trait Compatibility
 * @brief Compatibility shims for different C++ standard library implementations
 */

/**
 * @ingroup TypeTraits
 * @brief Platform-independent access to `is_trivially_destructible` trait.
 *
 * Different C++ standard libraries (libc++, libstdc++, MSVC STL) exposed
 * this trait at different times. This provides a compatibility layer.
 *
 * - libc++ with type_traits support: uses std::is_trivially_destructible
 * - libstdc++ >= 4.8: uses std::is_trivially_destructible
 * - MSVC STL: uses std::is_trivially_destructible
 * - Older libstdc++: aliases to std::has_trivial_destructor
 */
#define GLIBCXX_4_8 20130322

#if defined(_LIBCPP_TYPE_TRAITS) || defined(_MSC_VER) || __GLIBCXX__ >= GLIBCXX_4_8
// libc++ with type traights support, visual studio and libstdc++ >= 4.8
using std::is_trivially_destructible;
#else
#define is_trivially_destructible std::has_trivial_destructor
#endif

#endif /* THREAD_LOCAL_HPP_ */
