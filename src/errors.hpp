
#ifndef __ERRORS_HPP__
#define __ERRORS_HPP__

#include <errno.h>
#include <stdio.h>
#include <cstdlib>
#include <signal.h>
#include <stdexcept>

// #if LINUX x86
#define BREAKPOINT __asm__ volatile ("int3")
//#define BREAKPOINT raise(SIGTRAP);
// #endif

/* Error handling   (RSI)

There are several ways to report errors in RethinkDB:
   *   fail(msg, ...) always fails and reports line number and such
   *   assert(cond) makes sure cond is true and is a no-op in release mode
   *   check(msg, cond) makes sure cond is true. Its first two arguments should be switched but
       it's a legacy thing.
*/

#define fail_due_to_user_error fail
#define fail(msg, ...) do {                                         \
        report_fatal_error(__FILE__, __LINE__, msg, ##__VA_ARGS__); \
        abort();                                                    \
    } while (0)

#define fail_or_trap(msg, ...) do {                                 \
        report_fatal_error(__FILE__, __LINE__, msg, ##__VA_ARGS__); \
        BREAKPOINT;                                                 \
    } while (0)

void report_fatal_error(const char*, int, const char*, ...);

#define check(msg, err) do {                                            \
        if ((err)) {                                                    \
            if (errno == 0) {                                           \
                fail((msg));                                            \
            } else {                                                    \
                fail(msg " (errno %d - %s)", errno, strerror(errno));   \
            }                                                           \
        }                                                               \
    } while (0)

#define stringify(x) #x

#define format_assert_message(assert_type, cond, msg) (assert_type " failed: [" stringify(cond) "] " msg)
#define guarantee(cond) guaranteef(cond, "", 0)
#define guaranteef(cond, msg, ...) do { \
        if (!(cond)) {                  \
            fail_or_trap(format_assert_message("Guarantee", cond, msg), ##__VA_ARGS__);   \
        }                               \
    } while (0)


#define guarantee_err(cond, msg, ...) do {                                      \
        if (!(cond)) {                                                          \
            if (errno == 0) {                                                   \
                fail_or_trap(format_assert_message("Guarantee", cond, msg));    \
            } else {                                                            \
                fail_or_trap(format_assert_message("Guarantee", cond,  " (errno %d - %s)" msg), errno, strerror(errno), ##__VA_ARGS__);    \
            }                                                                   \
        }                                                                       \
    } while (0)

#define unreachable(msg, ...) fail("Unreachable code: " msg, ##__VA_ARGS__)       // RSI
#define not_implemented(msg, ...) fail("Not implemented: " msg, ##__VA_ARGS__)    // RSI

#define assert(cond) assertf(cond, "", 0)

#ifdef NDEBUG
#define assertf(cond, msg, ...) ((void)(0))
#else
#define assertf(cond, msg, ...) do { if (!(cond)) { fail_or_trap(format_assert_message("Assertion", cond, msg), ##__VA_ARGS__); } } while (0);
#endif


#ifndef NDEBUG
void print_backtrace(FILE *out = stderr, bool use_addr2line = true);
char *demangle_cpp_name(const char *mangled_name);
#endif

/*
std::string format_string(const char * fmt, va_list args) {
}

std::string format_string(const char * fmt, ...) {
    char buffer[1024];

    va_list args;
    va_start(args);
    int length = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    return std::string(buffer, length);
}

template<typename T> T create_exception(const char * fmt, ...) {
}

struct rdb_runtime_error : public std::runtime_error {
    explicit rdb_runtime_error(const string& msg) : std::runtime_error(msg) { }
    explicit rdb_runtime_error(const char * fmt, ...) : 
};

struct io_error : public std::runtime_error {
    io_error() : std::runtime_error(std::string(strerror(errno))) { }
    explicit io_error(const string& msg) : std::runtime_error(msg) { }
};
*/
#endif /* __ERRORS_HPP__ */
