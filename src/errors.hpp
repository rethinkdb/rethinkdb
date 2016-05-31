// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef ERRORS_HPP_
#define ERRORS_HPP_

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string>

#include "arch/compiler.hpp"

#ifdef _WIN32
#include "windows.hpp"
#endif

#ifndef DISABLE_BREAKPOINTS
#ifdef __linux__
#if defined __i386 || defined __x86_64
#define BREAKPOINT __asm__ volatile ("int3")
#else   /* not x86/amd64 */
#define BREAKPOINT (raise(SIGTRAP))
#endif  /* x86/amd64 */
#elif defined(__MACH__)
#define BREAKPOINT (raise(SIGTRAP))
#elif defined(_WIN32)
#define BREAKPOINT DebugBreak()
#else
#error "BREAKPOINT not defined for this operating system"
#endif
#else /* Breakpoints Disabled */
#define BREAKPOINT
#endif /* DISABLE_BREAKPOINTS */

#define CT_ASSERT(e) static_assert(e, #e)

#ifndef NDEBUG
#define DEBUG_ONLY(...) __VA_ARGS__
#define DEBUG_ONLY_CODE(expr) do { expr; } while (0)
#else
#define DEBUG_ONLY(...)
#define DEBUG_ONLY_CODE(expr) ((void)(0))
#endif

/* Accessors to errno.
 * Please access errno *only* through these access functions.
 * Accessing errno directly is unsafe in the context of
 * coroutines because compiler optimizations can interfer with TLS, which
 * might be used for errno.
 * See thread_local.hpp for a more detailed explanation of the issue. */
int get_errno();
void set_errno(int new_errno);
/* The following line can be useful for identifying illegal direct access in our
 * code. However it cannot be turned on in general because some system headers use
 * errno and don't compile with this. */
//#pragma GCC poison errno

/* Error handling
 *
 * There are several ways to report errors in RethinkDB:
 *  fail_due_to_user_error(msg, ...)    fail and report line number/stack trace. Should only be used when the user
 *                                      is at fault (e.g. provided wrong database file name) and it is reasonable to
 *                                      fail instead of handling the problem more gracefully.
 *
 *  The following macros are used only for the cases of programmer error checking. For the time being they are also used
 *  for system error checking (especially the *_err variants).
 *
 *  crash(msg, ...)                 always fails and reports line number and such. Never returns.
 *  crash_or_trap(msg, ...)         same as above, but traps into debugger if it is present instead of terminating.
 *                                  That means that it possibly can return, and one can continue stepping through the code in the debugger.
 *                                  All off the rassert/guarantee functions use crash_or_trap.
 *  rassert(cond)                   makes sure cond is true and is a no-op in release mode
 *  rassert(cond, msg, ...)         ditto, with additional message and possibly some arguments used for formatting
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

#define crash_or_trap(msg, ...) do {                                \
        report_fatal_error(__FILE__, __LINE__, msg, ##__VA_ARGS__); \
        BREAKPOINT;                                                 \
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
#define guarantee(cond, ...) do {       \
        if (!(cond)) {                  \
            crash_or_trap(format_assert_message("Guarantee", cond) __VA_ARGS__); \
        }                               \
    } while (0)

#define guarantee_xerr(cond, err, msg, ...) do {                                \
        int guarantee_xerr_errsv = (err);                                       \
        if (!(cond)) {                                                          \
            if (guarantee_xerr_errsv == 0) {                                    \
                crash_or_trap(format_assert_message("Guarantee", cond) msg, ##__VA_ARGS__); \
            } else {                                                            \
                char guarantee_xerr_buf[250];                                      \
                const char *errstr = errno_string_maybe_using_buffer(guarantee_xerr_errsv, guarantee_xerr_buf, sizeof(guarantee_xerr_buf)); \
                crash_or_trap(format_assert_message("Guarantee", cond) " (errno %d - %s) " msg, guarantee_xerr_errsv, errstr, ##__VA_ARGS__); \
            }                                                                   \
        }                                                                       \
    } while (0)
#define guarantee_err(cond, ...) guarantee_xerr(cond, get_errno(), ##__VA_ARGS__)

#ifdef _WIN32

#define guarantee_xwinerr(cond, err, msg, ...) do {                     \
        if (!(cond)) {                                                  \
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
        if (!(cond)) {                                                    \
            crash_or_trap(format_assert_message("Assertion", cond) __VA_ARGS__); \
        }                                                                 \
    } while (0)
#define rassert_err(cond, msg, ...) do {                                    \
        int rassert_err_errsv = get_errno();                                      \
        if (!(cond)) {                                                      \
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


/* Put these after functions to indicate what they throw. In release mode, they
turn into noops so that the compiler doesn't have to generate exception-checking
code everywhere. If you need to add an exception specification for compatibility
with e.g. a virtual method, don't use these, or your code won't compile in
release mode. */
#ifdef NDEBUG
#define THROWS_NOTHING
#define THROWS_ONLY(...)
#else
#define THROWS_NOTHING throw ()
#define THROWS_ONLY(...) throw (__VA_ARGS__)
#endif

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
