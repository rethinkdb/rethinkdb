// Copyright 2010-2013 RethinkDB, all rights reserved.

/// @file errors.hpp
/// @brief Error handling, assertions, and debugging macros for RethinkDB
///
/// This comprehensive error handling module provides:
/// - Platform-specific breakpoint and trap instructions
/// - Assertion macros for debug and release builds
/// - Error reporting functions with stack traces
/// - Compiler optimizations hints (LIKELY/UNLIKELY)
/// - Safe errno access through thread-local storage
///
/// @note This is a foundational header used across the entire codebase
/// @defgroup ErrorHandling Error Handling and Assertions
/// Macros and functions for reporting errors and validating assertions
/// @{

#ifndef ERRORS_HPP_
#define ERRORS_HPP_

#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string>

#include "arch/compiler.hpp"

#ifdef _WIN32
#include "windows.hpp"
#endif

/// @brief Platform-specific breakpoint for debugger
/// Inserts a breakpoint instruction that is recognized by debuggers
/// @note Can be disabled by defining DISABLE_BREAKPOINTS before including this header
#ifndef DISABLE_BREAKPOINTS
#ifdef __linux__
#if defined __i386 || defined __x86_64
/// @internal x86/x86-64 breakpoint instruction
#define BREAKPOINT __asm__ volatile ("int3")
#else   /* not x86/amd64 */
/// @internal Generic POSIX breakpoint via signal
#define BREAKPOINT (raise(SIGTRAP))
#endif  /* x86/amd64 */
#elif defined(__MACH__) || defined(__FreeBSD__)
/// @internal macOS/FreeBSD breakpoint via SIGTRAP
#define BREAKPOINT (raise(SIGTRAP))
#elif defined(_WIN32)
/// @internal Windows breakpoint function
#define BREAKPOINT DebugBreak()
#else
#error "BREAKPOINT not defined for this operating system"
#endif
#else /* Breakpoints Disabled */
/// @internal Breakpoint disabled - no-op
#define BREAKPOINT
#endif /* DISABLE_BREAKPOINTS */

/// @brief Compile-time assertion macro
/// Verifies expressions at compile time; fails with clear error message if false
/// @param e The expression to check (must be compile-time constant)
/// @example
/// @code
/// CT_ASSERT(sizeof(uint32_t) == 4);  // Verify platform assumptions
/// @endcode
#define CT_ASSERT(e) static_assert(e, #e)

/// @brief Conditionally defined code for debug builds
/// Expands to arguments in debug builds, empty in release builds
/// @param ... Code to include only in debug builds
/// @note Only used internally; prefer DEBUG_ONLY_CODE for executable statements
/// @example
/// @code
/// #ifdef NDEBUG
/// // ... release code ...
/// #else
/// DEBUG_ONLY(int debug_count = 0;)  // Only compiled in debug mode
/// #endif
/// @endcode
#ifndef NDEBUG
#define DEBUG_ONLY(...) __VA_ARGS__

/// @brief Debug-only executable statements
/// Executes the provided expression only in debug builds
/// @param expr Expression to execute only in debug mode
/// @example
/// @code
/// DEBUG_ONLY_CODE(validate_data_structure());
/// @endcode
#define DEBUG_ONLY_CODE(expr) do { expr; } while (0)
#else
#define DEBUG_ONLY(...)
#define DEBUG_ONLY_CODE(expr) ((void)(0))
#endif

/// @brief Compiler branch prediction hint for false branch
/// Hints to the compiler that a condition is unlikely to be true,
/// enabling better code optimization
/// @param x The condition to evaluate
/// @return The value of x
/// @example
/// @code
/// if (UNLIKELY(error_code == -1)) {
///     handle_error();  // This branch is rarely taken
/// }
/// @endcode
#if defined __clang__ || defined __GNUC__
#define UNLIKELY(x) __builtin_expect(x, 0)
#else
#define UNLIKELY(x) x
#endif

/// @brief Gets the current value of errno in a thread-safe manner
/// Accesses errno through thread-local storage to avoid compiler optimizations
/// that could interfere with coroutine contexts
/// @return The current errno value
/// @see set_errno, Error handling overview
int get_errno();

/// @brief Sets errno to a new value in a thread-safe manner
/// Updates errno through thread-local storage for safety with coroutines
/// @param new_errno The new errno value to set
/// @see get_errno, Error handling overview
void set_errno(int new_errno);

/// @brief Documented error reporting strategy for RethinkDB
/// RethinkDB uses multiple error handling approaches depending on context:
///
/// - `fail_due_to_user_error(msg, ...)`: Fail with traceback when user provided
///   invalid input (e.g., wrong database file path). Prefer this for recoverable
///   user errors only.
///
/// - `crash(msg, ...)`: Immediate termination with traceback. Used for
///   unrecoverable programmer errors. Never returns.
///
/// - `crash_or_trap(msg, ...)`: Like crash() but traps debugger if attached,
///   allowing step-through debugging. Returns if debugger continues execution.
///   All rassert/guarantee functions use this.
///
/// - `rassert(cond)`: Debug-only assertion that becomes no-op in release.
///
/// - `rassert(cond, msg, ...)`: Debug assertion with formatted message.
///
/// @example
/// @code
/// // User error - invalid configuration
/// if (config.buffer_size < 1024) {
///     fail_due_to_user_error("Buffer size must be at least 1024 bytes");
/// }
///
/// // Programmer error - something that should never happen
/// if (nullptr == critical_pointer) {
///     crash("Critical pointer is null - memory corruption?");
/// }
///
/// // Debug-time validation
/// rassert(list_size >= 0, "List size is negative: %d", list_size);
/// @endcode
/// @defgroup ErrorReporting Error Reporting Functions
/// Functions for reporting and handling various types of errors
/// @{

/// @}
 *  rassert_err(cond)               same as rassert(cond), but also print errno error description
 *  rassert_err(cond, msg, ...)     same as rassert(cond, msg, ...), but also print errno error description
 *  guarantee(cond)                 same as rassert(cond), but the check is still done in release mode. Do not use for expensive checks!
 *  guarantee(cond, msg, ...)       same as rassert(cond, msg, ...), but the check is still done in release mode. Do not use for expensive checks!
 *  guarantee_err(cond)             same as guarantee(cond), but also print errno error description
 *  guarantee_err(cond, msg, ...)   same as guarantee(cond, msg, ...), but also print errno error description
 *  guarantee_xerr(cond, err, msg, ...) same as guarantee_err(cond, msg, ...), but also allows to specify errno as err argument
 *                                  (useful for async io functions, which return negated errno)
 *
 * The names rassert* are used instead of assert* because /usr/include/assert.h undefines assert macro and redefines it with its own version
 * every single time it gets included.
 */

#ifndef NDEBUG
#define DEBUG_VAR
#else
#define DEBUG_VAR UNUSED
#endif

#define fail_due_to_user_error(msg, ...) do {  \
        report_user_error(msg, ##__VA_ARGS__); \
        BREAKPOINT;                            \
        exit(EXIT_FAILURE);                    \
    } while (0)

#define crash(msg, ...) do {                                        \
        report_fatal_error(__FILE__, __LINE__, msg, ##__VA_ARGS__); \
        BREAKPOINT; /* this used to be abort(), but it didn't cause VALGRIND to print a backtrace */ \
        ::abort();                                                  \
    } while (0)

// We added abort() to this -- sorry if this hurts your debug tracing
// -- raise(SIGTRAP) is not terminating the process on MacOS anymore,
// probably because we are blocking it in thread_pool.cc.
#define crash_or_trap(msg, ...) do {                                \
        report_fatal_error(__FILE__, __LINE__, msg, ##__VA_ARGS__); \
        BREAKPOINT;                                                 \
        ::abort();                                                  \
    } while (0)

void report_fatal_error(const char*, int, const char*, ...) ATTR_FORMAT(printf, 3, 4);
void report_user_error(const char*, ...) ATTR_FORMAT(printf, 1, 2);

// Our usual crash() method does not work well in out-of-memory conditions, because
// it performs heap-allocations itself. Use `crash_oom()` instead for these cases.
NORETURN void crash_oom();

// Possibly using buf to store characters, returns a pointer to a strerror-style error string.  This
// has the same contract as the GNU (char *)-returning strerror_r.  The return value is a pointer to
// a nul-terminated string, either equal to buf or pointing at a statically allocated string.
MUST_USE const char *errno_string_maybe_using_buffer(int errsv, char *buf, size_t buflen);

#ifdef _WIN32
MUST_USE const std::string winerr_string(DWORD winerr);
#endif

#define stringify(x) #x

#define format_assert_message(assert_type, cond) assert_type " failed: [" stringify(cond) "] "
#define guarantee(cond, ...) do {                                                \
        if (UNLIKELY(!(cond))) {                                                 \
            crash_or_trap(format_assert_message("Guarantee", cond) __VA_ARGS__); \
        }                                                                        \
    } while (0)

#define guarantee_xerr(cond, err, msg, ...) do {                                \
        int guarantee_xerr_errsv = (err);                                       \
        if (UNLIKELY(!(cond))) {                                                \
            if (guarantee_xerr_errsv == 0) {                                    \
                crash_or_trap(format_assert_message("Guarantee", cond) msg, ##__VA_ARGS__); \
            } else {                                                            \
                char guarantee_xerr_buf[250];                                   \
                const char *errstr = errno_string_maybe_using_buffer(guarantee_xerr_errsv, guarantee_xerr_buf, sizeof(guarantee_xerr_buf));   \
                crash_or_trap(format_assert_message("Guarantee", cond) " (errno %d - %s) " msg, guarantee_xerr_errsv, errstr, ##__VA_ARGS__); \
            }                                                                   \
        }                                                                       \
    } while (0)
#define guarantee_err(cond, ...) guarantee_xerr(cond, get_errno(), ##__VA_ARGS__)

#ifdef _WIN32

#define guarantee_xwinerr(cond, err, msg, ...) do {                     \
        if (UNLIKELY(!(cond))) {                                        \
            DWORD guarantee_winerr_err = (err);                         \
            crash_or_trap(format_assert_message("Guarantee", (cond)) "(error 0x%x - %s) " msg, guarantee_winerr_err, winerr_string(guarantee_winerr_err).c_str(), ##__VA_ARGS__); \
        }                                                               \
    } while (0)

#define guarantee_winerr(cond, ...) guarantee_xwinerr(cond, GetLastError(), ##__VA_ARGS__)

#endif

#define unreachable(...) crash("Unreachable code: " __VA_ARGS__)    // can't use crash_or_trap since code needs to depend on its noreturn property

#ifdef NDEBUG
#define rassert(cond, ...) ((void)(0))
#define rassert_err(cond, ...) ((void)(0))
#else
#define rassert(cond, ...) do {                                           \
        if (UNLIKELY(!(cond))) {                                          \
            crash_or_trap(format_assert_message("Assertion", cond) __VA_ARGS__); \
        }                                                                 \
    } while (0)
#define rassert_err(cond, msg, ...) do {                                    \
        if (UNLIKELY(!(cond))) {                                            \
            int rassert_err_errsv = get_errno();                            \
            if (rassert_err_errsv == 0) {                                   \
                crash_or_trap(format_assert_message("Assert", cond) msg);   \
            } else {                                                        \
                char rassert_err_buf[250];                                  \
                const char *errstr = errno_string_maybe_using_buffer(rassert_err_errsv, rassert_err_buf, sizeof(rassert_err_buf)); \
                crash_or_trap(format_assert_message("Assert", cond) " (errno %d - %s) " msg, rassert_err_errsv, errstr, ##__VA_ARGS__);  \
            }                                                               \
        }                                                                   \
    } while (0)
#endif


void install_generic_crash_handler();
void install_new_oom_handler();

// If you include errors.hpp before including a Boost library, then Boost assertion
// failures will be forwarded to the RethinkDB error mechanism.
#define BOOST_ENABLE_ASSERT_HANDLER
namespace boost {
void assertion_failed(char const * expr, char const * function, char const * file, long line);  // NOLINT(runtime/int)
}

#define DISABLE_COPYING(T)                      \
    T(const T&) = delete;                       \
    T& operator=(const T&) = delete

#define MOVABLE_BUT_NOT_COPYABLE(T) \
    DISABLE_COPYING(T);             \
    T(T &&) = default;              \
    T &operator=(T &&) = default


/* Put these after functions to indicate what they throw. In all modes, they
turn into noops so that the compiler doesn't have to generate exception-checking
code everywhere.  Originally, these resolved to throw() and throw(__VA_ARGS__)
in debug mode, but C++17 removed the language feature.  Consider these to be
vestigial. */
#define THROWS_NOTHING
#define THROWS_ONLY(...)

// This is a workaround for old versions of boost causing a compilation error
#include <boost/version.hpp> // NOLINT(build/include_order)
#if (BOOST_VERSION >= 104200) && (BOOST_VERSION <= 104399)
#include <boost/config.hpp> // NOLINT(build/include_order)
#undef BOOST_HAS_RVALUE_REFS
#endif

#ifdef __GNUC__
#define GNUC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif


/** RVALUE_THIS
 *
 * This macro is used to annotate methods that treat *this as an
 * rvalue reference. On compilers that support it, it expands to &&
 * and all uses of the method on non-rvlaue *this are reported as
 * errors.
 *
 * The supported compilers are clang >= 2.9 and gcc >= 4.8.1
 *
 **/
#if defined(__clang__)
#if __has_extension(cxx_rvalue_references)
#define RVALUE_THIS &&
#else
#define RVALUE_THIS
#endif
#elif __GNUC__ > 4 || (__GNUC__ == 4 && \
    (__GNUC_MINOR__ > 8 || (__GNUC_MINOR__ == 8 && \
                            __GNUC_PATCHLEVEL__ > 1)))
#define RVALUE_THIS &&
#elif defined(_MSC_VER)
#define RVALUE_THIS &&
#else
#define RVALUE_THIS
#endif


#if defined(__clang__)
    #if !__has_extension(cxx_override_control)
        #define override
        #define final
    #endif
#elif defined(__GNUC__) && GNUC_VERSION < 40700
    #define override
    #define final
#endif

#endif /* ERRORS_HPP_ */
