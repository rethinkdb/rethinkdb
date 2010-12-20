
#ifndef __ERRORS_HPP__
#define __ERRORS_HPP__

#include <errno.h>
#include <stdio.h>
#include <cstdlib>
#include <signal.h>
#include <stdexcept>

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
 *                                  All off the assert/guarantee functions use crash_or_trap.
 *  assert(cond)                    makes sure cond is true and is a no-op in release mode
 *  assert(cond, msg, ...)          ditto, with additional message and possibly some arguments used for formatting
 *  assert_err(cond)                same as assert(cond), but also print errno error description
 *  assert_err(cond, msg, ...)      same as assert(cond, msg, ...), but also print errno error description
 *  guarantee(cond)                 same as assert(cond), but the check is still done in release mode. Do not use for expensive checks!
 *  guarantee(cond, msg, ...)       same as assert(cond, msg, ...), but the check is still done in release mode. Do not use for expensive checks!
 *  guarantee_err(cond)             same as guarantee(cond), but also print errno error description
 *  guarantee_err(cond, msg, ...)   same as guarantee(cond, msg, ...), but also print errno error description
 *  guarantee_xerr(cond, err, msg, ...) same as guarantee_err(cond, msg, ...), but also allows to specify errno as err argument
 *                                  (useful for async io functions, which return negated errno)
*/

#ifdef __linux__
#if defined __i386 || defined __x86_64
#define BREAKPOINT __asm__ volatile ("int3")
#else   /* x86/amd64 */
#define BREAKPOINT raise(SIGTRAP)
#endif  /* x86/amd64 */
 #endif /* __linux__ */

// TODO: Abort probably is not the right thing to do here.
#define fail_due_to_user_error(msg, ...) do {                       \
        report_user_error(msg, ##__VA_ARGS__);                                     \
        exit(-1);                                                    \
    } while (0)

#define crash(msg, ...) do {                                        \
        report_fatal_error(__FILE__, __LINE__, msg, ##__VA_ARGS__); \
        abort();                                                    \
    } while (0)

#define crash_or_trap(msg, ...) do {                                \
        report_fatal_error(__FILE__, __LINE__, msg, ##__VA_ARGS__); \
        BREAKPOINT;                                                 \
    } while (0)

void report_fatal_error(const char*, int, const char*, ...);
void report_user_error(const char*, ...);

#define stringify(x) #x

#define format_assert_message(assert_type, cond) assert_type " failed: [" stringify(cond) "] "
#define guarantee(cond, msg...) do {    \
        if (!(cond)) {                  \
            crash_or_trap(format_assert_message("Guarantee", cond) msg); \
        }                               \
    } while (0)
#define guarantee_xerr(cond, err, msg, args...) do {                            \
        if (!(cond)) {                                                          \
            if (err == 0) {                                                     \
                crash_or_trap(format_assert_message("Guarantee", cond) msg);    \
            } else {                                                            \
                crash_or_trap(format_assert_message("Guarantee", cond) " (errno %d - %s) " msg, err, strerror(err), ##args);  \
            }                                                                   \
        }                                                                       \
    } while (0)
#define guarantee_err(cond, msg, args...) guarantee_xerr(cond, errno, msg, ##args)

#define unreachable(msg, ...) crash("Unreachable code: " msg, ##__VA_ARGS__)    // can't use crash_or_trap since code needs to depend on its noreturn property
#define not_implemented(msg, ...) crash_or_trap("Not implemented: " msg, ##__VA_ARGS__)

/* Another way to mark unused variables:
 *   int foo __attribute__((unused)) = bar();
 */
#define UNUSED(x) ((void) x)

#ifdef NDEBUG
#define assert(cond, msg...) ((void)(0))
#define assert_err(cond, msg...) ((void)(0))
#else
#define assert(cond, msg...) do {   \
        if (!(cond)) {              \
            crash_or_trap(format_assert_message("Assertion", cond) msg); \
        }                           \
    } while (0)
#define assert_err(cond, msg, args...) do {                                 \
        if (!(cond)) {                                                      \
            if (errno == 0) {                                               \
                crash_or_trap(format_assert_message("Assert", cond) msg);   \
            } else {                                                        \
                crash_or_trap(format_assert_message("Assert", cond) " (errno %d - %s) " msg, errno, strerror(errno), ##args);  \
            }                                                               \
        }                                                                   \
    } while (0)
#endif


#ifndef NDEBUG
void print_backtrace(FILE *out = stderr, bool use_addr2line = true);
char *demangle_cpp_name(const char *mangled_name, char* buffer = NULL, size_t* buffer_length = NULL);
#endif

#endif /* __ERRORS_HPP__ */
