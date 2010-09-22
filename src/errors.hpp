
#ifndef __ERRORS_HPP__
#define __ERRORS_HPP__

#include <stdio.h>

/* Error handling

There are several ways to report errors in RethinkDB:
   *   fail(msg, ...) always fails and reports line number and such
   *   assert(cond) makes sure cond is true and is a no-op in release mode
   *   check(msg, cond, ...) makes sure cond is true. Its first two arguments should be switched but
       it's a legacy thing.
*/

#define fail(...) _fail(__FILE__, __LINE__, __VA_ARGS__)
void _fail(const char*, int, const char*, ...) __attribute__ ((noreturn));

#define check(msg, err) \
    ((err) ? \
        (errno ? \
            fail((msg)) : \
            fail(msg " (errno = %s)", strerror(errno)) \
            ) : \
        (void)(0) \
        )

#define stringify(x) #x
#define assert(cond) assertf(cond, "Assertion failed: " stringify(cond))

#ifdef NDEBUG
#define assertf(cond, ...) ((void)(0))
#else
#define assertf(cond, ...) ((cond) ? (void)(0) : fail(__VA_ARGS__))
#endif

#ifndef NDEBUG
void print_backtrace(FILE *out = stderr);
char *demangle_cpp_name(const char *mangled_name);
#endif

#endif /* __ERRORS_HPP__ */
