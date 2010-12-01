
#ifndef __ERRORS_HPP__
#define __ERRORS_HPP__

#include <errno.h>
#include <stdio.h>
#include <cstdlib>
#include <signal.h>

// #if LINUX x86
//#define BREAKPOINT __asm__ volatile ("int3")
#define BREAKPOINT raise(SIGTRAP);
// #endif

/* Error handling

There are several ways to report errors in RethinkDB:
   *   fail(msg, ...) always fails and reports line number and such
   *   assert(cond) makes sure cond is true and is a no-op in release mode
   *   check(msg, cond) makes sure cond is true. Its first two arguments should be switched but
       it's a legacy thing.
*/

#define fail(...) do {                                          \
        report_fatal_error(__FILE__, __LINE__, __VA_ARGS__);    \
        abort();                                                \
    } while (0);

#define fail_or_trap(...) do {                                  \
        report_fatal_error(__FILE__, __LINE__, __VA_ARGS__);    \
        BREAKPOINT;                                             \
    } while (0);

void report_fatal_error(const char*, int, const char*, ...);

#define check(msg, err) do {                                            \
        if ((err)) {                                                    \
            if (errno == 0) {                                           \
                fail((msg));                                            \
            } else {                                                    \
                fail(msg " (errno %d - %s)", errno, strerror(errno));   \
            }                                                           \
        }                                                               \
    } while (0);

#define stringify(x) #x

#define guarantee(cond) guaranteef(cond, "Guarantee failed: " stringify(cond))
#define guaranteef(cond, ...) do { if (!(cond)) { fail_or_trap(__VA_ARGS__); } } while (0);

#define assert(cond) assertf(cond, "Assertion failed: " stringify(cond))

#ifdef NDEBUG
#define assertf(cond, ...) ((void)(0))
#else
#define assertf(cond, ...) do { if (!(cond)) { fail_or_trap(__VA_ARGS__); } } while (0);
#endif


#ifndef NDEBUG
void print_backtrace(FILE *out = stderr, bool use_addr2line = true);
char *demangle_cpp_name(const char *mangled_name);
#endif

#endif /* __ERRORS_HPP__ */
