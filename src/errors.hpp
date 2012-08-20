#ifndef ERRORS_HPP_
#define ERRORS_HPP_

#include <errno.h>
#include <stdlib.h>

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

// A macro that expands to nothing.
#define EXPAND_TO_NOTHING

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
#define DEBUG_ONLY_VAR
#else
#define DEBUG_ONLY_VAR __attribute__((unused))
#endif

#define UNUSED __attribute__((unused))
#define MUST_USE __attribute__((warn_unused_result))

#define fail_due_to_user_error(...) do {                                \
        report_user_error(__VA_ARGS__);                                 \
        exit(-1);                                                       \
    } while (0)

#define crash(...) do {                                                 \
        report_fatal_error(__FILE__, __LINE__, __VA_ARGS__);            \
        BREAKPOINT; /* this used to be abort(), but it didn't cause VALGRIND to print a backtrace */ \
        abort();                                                    \
    } while (0)

#define crash_or_trap(...) do {                                     \
        report_fatal_error(__FILE__, __LINE__, __VA_ARGS__);        \
        BREAKPOINT;                                                 \
    } while (0)

#define crash_or_trap_err(err, ...) do {                                \
        report_fatal_errno_error(__FILE__, __LINE__, err, __VA_ARGS__); \
        BREAKPOINT;                                                     \
    } while (0)

void report_fatal_errno_error(const char *file, int line, int errsv, const char *msg, ...) __attribute__((format (printf, 4, 5)));
void report_fatal_error(const char*, int, const char*, ...) __attribute__((format (printf, 3, 4)));
void report_user_error(const char*, ...) __attribute__((format (printf, 1, 2)));

#define stringify(x) #x

#define format_assert_message(assert_type, cond) assert_type " failed: [" stringify(cond) "] "
#define guaranteef(cond, msg...) do {    \
        if (!(cond)) {                  \
            crash_or_trap(format_assert_message("Guarantee", cond) msg); \
        }                               \
    } while (0)
#define guarantee(cond) guaranteef(cond, "")
#define guarantee_xerr(cond, err, ...) do {                             \
        if (!(cond)) {                                                  \
            if ((err) == 0) {                                           \
                crash_or_trap(format_assert_message("Guarantee", cond) __VA_ARGS__); \
            } else {                                                    \
                crash_or_trap_err((err), format_assert_message("Guarantee", cond) __VA_ARGS__); \
            }                                                           \
        }                                                               \
    } while (0)
#define guaranteef_err(cond, ...) guarantee_xerr(cond, errno, __VA_ARGS__)
#define guarantee_err(cond) guarantee_err(cond, "");

#define unreachable(...) crash("Unreachable code: " __VA_ARGS__)    // can't use crash_or_trap since code needs to depend on its noreturn property
#define not_implemented(...) crash_or_trap("Not implemented: " __VA_ARGS__)

#ifdef NDEBUG
#define rassertf(cond, msg...) ((void)(0))
#define rassertf(cond) ((void)(0))
#define rassert_err(cond, msg...) ((void)(0))
#define rassertf_err(cond, msg...) ((void)(0))
#else
#define rassertf(cond, msg...) do {                                        \
        if (!(cond)) {                                                    \
            crash_or_trap(format_assert_message("Assertion", cond) msg);  \
        }                                                                 \
    } while (0)
#define rassert(cond) rassertf(cond, "")
#define rassertf_err(cond, ...) do {                                    \
        if (!(cond)) {                                                  \
            if (errno == 0) {                                           \
                crash_or_trap(format_assert_message("Assert", cond) __VA_ARGS__); \
            } else {                                                        \
                crash_or_trap_err(errno, format_assert_message("Assert", cond) __VA_ARGS__); \
            }                                                               \
        }                                                                   \
    } while (0)
#define rassert_err(cond) rassertf_err(cond, "")
#endif

void install_generic_crash_handler();

// If you include errors.hpp before including a Boost library, then Boost assertion
// failures will be forwarded to the RethinkDB error mechanism.
#define BOOST_ENABLE_ASSERT_HANDLER
namespace boost {
void assertion_failed(char const * expr, char const * function, char const * file, long line);  // NOLINT(runtime/int)
}


// put this in a private: section.
#define DISABLE_COPYING(T)                      \
    T(const T&);                                \
    void operator=(const T&)


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
