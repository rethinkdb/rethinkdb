// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef ERRORS_HPP_
#define ERRORS_HPP_

#include <errno.h>
#include <signal.h>
#include <stdlib.h>

// Boost 1.53.0 would not build on OS X (using Apple's Clang 4.0, which is based
// off of clang 3.1svn).  See https://github.com/rethinkdb/rethinkdb/issues/377
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
#error "BOOST_NO_CXX11_RVALUE_REFERENCES should have been defined by the Makefile."
#endif

#ifndef DISABLE_BREAKPOINTS
#ifdef __linux__
#if defined __i386 || defined __x86_64
#define BREAKPOINT __asm__ volatile ("int3")
#else   /* not x86/amd64 */
#define BREAKPOINT raise(SIGTRAP)
#endif  /* x86/amd64 */
#endif /* __linux__ */

#ifdef __MACH__
#define BREAKPOINT raise(SIGTRAP)
#endif
#else /* Breakpoints Disabled */
#define BREAKPOINT
#endif /* DISABLE_BREAKPOINTS */

#define CT_ASSERT(e) {enum { compile_time_assert_error = 1/(!!(e)) };}

#ifndef NDEBUG
#define DEBUG_ONLY(...) __VA_ARGS__
#define DEBUG_ONLY_CODE(expr) do { expr; } while (0)
#else
#define DEBUG_ONLY(...)
#define DEBUG_ONLY_CODE(expr) ((void)(0))
#endif

/* This macro needs to exist because gcc and icc disagree on how to number the
 * attributes to methods. ICC does different number for methods and functions
 * and it's too unwieldy to make programs use these macros so we're just going
 * to disable them in icc.
 * With this macro attribute number starts at 1. If it is a nonstatic method
 * then "1" refers to the class pointer ("this").
 * */
#ifdef __ICC
#define NON_NULL_ATTR(arg)
#else
#define NON_NULL_ATTR(arg) __attribute__((nonnull(arg)))
#endif

#define NORETURN __attribute__((noreturn))

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
#define DEBUG_VAR __attribute__((unused))
#endif

#define UNUSED __attribute__((unused))

#define MUST_USE __attribute__((warn_unused_result))

#define fail_due_to_user_error(msg, ...) do {  \
        report_user_error(msg, ##__VA_ARGS__); \
        BREAKPOINT;                            \
        exit(EXIT_FAILURE);                    \
    } while (0)

#define crash(msg, ...) do {                                        \
        report_fatal_error(__FILE__, __LINE__, msg, ##__VA_ARGS__); \
        BREAKPOINT; /* this used to be abort(), but it didn't cause VALGRIND to print a backtrace */ \
        abort();                                                    \
    } while (0)

#define nice_crash(...) do {          \
        fprintf(stderr, __VA_ARGS__); \
        exit(EXIT_FAILURE);           \
    } while (0)

#define crash_or_trap(msg, ...) do {                                \
        report_fatal_error(__FILE__, __LINE__, msg, ##__VA_ARGS__); \
        BREAKPOINT;                                                 \
    } while (0)

void report_fatal_error(const char*, int, const char*, ...) __attribute__((format (printf, 3, 4)));
void report_user_error(const char*, ...) __attribute__((format (printf, 1, 2)));

// Possibly using buf to store characters, returns a pointer to a strerror-style error string.  This
// has the same contract as the GNU (char *)-returning strerror_r.  The return value is a pointer to
// a nul-terminated string, either equal to buf or pointing at a statically allocated string.
MUST_USE const char *errno_string_maybe_using_buffer(int errsv, char *buf, size_t buflen);

#define stringify(x) #x

#define format_assert_message(assert_type, cond) assert_type " failed: [" stringify(cond) "] "
#define guarantee(cond, msg...) do {    \
        if (!(cond)) {                  \
            crash_or_trap(format_assert_message("Guarantee", cond) msg); \
        }                               \
    } while (0)

#define nice_guarantee(cond, msg, ...) do { \
        if (!(cond)) {                      \
            nice_crash(msg, ##__VA_ARGS__); \
        }                                   \
    } while (0)

#define guarantee_xerr(cond, err, msg, args...) do {                            \
        int guarantee_xerr_errsv = (err);                                       \
        if (!(cond)) {                                                          \
            if (guarantee_xerr_errsv == 0) {                                    \
                crash_or_trap(format_assert_message("Guarantee", cond) msg, ##args); \
            } else {                                                            \
                char guarantee_xerr_buf[250];                                      \
                const char *errstr = errno_string_maybe_using_buffer(guarantee_xerr_errsv, guarantee_xerr_buf, sizeof(guarantee_xerr_buf)); \
                crash_or_trap(format_assert_message("Guarantee", cond) " (errno %d - %s) " msg, guarantee_xerr_errsv, errstr, ##args); \
            }                                                                   \
        }                                                                       \
    } while (0)
#define guarantee_err(cond, msg, args...) guarantee_xerr(cond, errno, msg, ##args)

#define unreachable(msg, ...) crash("Unreachable code: " msg, ##__VA_ARGS__)    // can't use crash_or_trap since code needs to depend on its noreturn property
#define not_implemented(msg, ...) crash_or_trap("Not implemented: " msg, ##__VA_ARGS__)

#ifdef NDEBUG
#define rassert(cond, msg...) ((void)(0))
#define rassert_err(cond, msg...) ((void)(0))
#else
#define rassert(cond, msg...) do {                                        \
        if (!(cond)) {                                                    \
            crash_or_trap(format_assert_message("Assertion", cond) msg);  \
        }                                                                 \
    } while (0)
#define rassert_err(cond, msg, args...) do {                                \
        int rassert_err_errsv = errno;                                      \
        if (!(cond)) {                                                      \
            if (rassert_err_errsv == 0) {                                   \
                crash_or_trap(format_assert_message("Assert", cond) msg);   \
            } else {                                                        \
                char rassert_err_buf[250];                                  \
                const char *errstr = errno_string_maybe_using_buffer(rassert_err_errsv, rassert_err_buf, sizeof(rassert_err_buf)); \
                crash_or_trap(format_assert_message("Assert", cond) " (errno %d - %s) " msg, rassert_err_errsv, errstr, ##args);  \
            }                                                               \
        }                                                                   \
    } while (0)
#endif


void install_generic_crash_handler();

// If you include errors.hpp before including a Boost library, then Boost assertion
// failures will be forwarded to the RethinkDB error mechanism.
#define BOOST_ENABLE_ASSERT_HANDLER
namespace boost {
void assertion_failed(char const * expr, char const * function, char const * file, long line);  // NOLINT(runtime/int)
}

#ifdef MCHECK
void mcheck_init(void);
void mcheck_all(void);
#endif

#define DISABLE_COPYING(T)                      \
    T(const T&) = delete;                       \
    T& operator=(const T&) = delete


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


#endif /* ERRORS_HPP_ */
