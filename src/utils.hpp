// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef UTILS_HPP_
#define UTILS_HPP_

#include <stdio.h>
#include <stdlib.h>

#include <array>
#include <functional>
#include <string>

#include "debug.hpp"
#include "arch/compiler.hpp"
#include "config/args.hpp"
#include "errors.hpp"

#ifdef _MSC_VER
#include <BaseTsd.h>
#include <random>
typedef SSIZE_T ssize_t;
#endif

class printf_buffer_t;

namespace ph = std::placeholders;

class startup_shutdown_t {
public:
    startup_shutdown_t();
    ~startup_shutdown_t();
};

struct const_charslice {
    const char *beg, *end;
    const_charslice(const char *_beg, const char *_end) : beg(_beg), end(_end) { }
    const_charslice() : beg(nullptr), end(nullptr) { }
};

void *raw_malloc_aligned(size_t size, size_t alignment);
void raw_free_aligned(void *ptr);

#ifndef _WIN32
void *raw_malloc_page_aligned(size_t size);
#endif

/* Calls `malloc()` and checks its return value to crash if the allocation fails. */
void *rmalloc(size_t size);
/* Calls `realloc()` and checks its return value to crash if the allocation fails. */
void *rrealloc(void *ptr, size_t size);

/* Forwards to the isfinite macro, or std::isfinite. */
bool risfinite(double);

enum class query_state_t { FAILED, INDETERMINATE };

// Reads from /dev/urandom.  Use this sparingly, please.
void system_random_bytes(void *out, int64_t nbytes);

bool begins_with_minus(const char *string);
// strtoul() and strtoull() will for some reason not fail if the input begins
// with a minus sign. strtou64_strict() does.  Also we fix the constness of the
// end parameter.
int64_t strtoi64_strict(const char *string, const char **end, int base);
uint64_t strtou64_strict(const char *string, const char **end, int base);

// These functions return false and set the result to 0 if the conversion fails or
// does not consume the whole string.
MUST_USE bool strtoi64_strict(const std::string &str, int base, int64_t *out_result);
MUST_USE bool strtou64_strict(const std::string &str, int base, uint64_t *out_result);

std::string strprintf(const char *format, ...) ATTR_FORMAT(printf, 1, 2);
std::string vstrprintf(const char *format, va_list ap) ATTR_FORMAT(printf, 1, 0);


// formatted time:
// yyyy-mm-ddThh:mm:ss.nnnnnnnnn   (29 characters)
const size_t formatted_time_length = 29;    // not including null

enum class local_or_utc_time_t { local, utc };
void format_time(struct timespec time, printf_buffer_t *buf, local_or_utc_time_t zone);
std::string format_time(struct timespec time, local_or_utc_time_t zone);

bool parse_time(
    const std::string &str, local_or_utc_time_t zone,
    struct timespec *out, std::string *errmsg_out);

/* Printing binary data to stderr in a nice format */
void print_hd(const void *buf, size_t offset, size_t length);



/* `with_priority_t` changes the priority of the current coroutine to the
 value given in its constructor. When it is destructed, it restores the
 original priority of the coroutine. */

class with_priority_t {
public:
    explicit with_priority_t(int priority);
    ~with_priority_t();
private:
    int previous_priority;
};


template <class InputIterator, class UnaryPredicate>
bool all_match_predicate(InputIterator begin, InputIterator end, UnaryPredicate f) {
    bool res = true;
    for (; begin != end; begin++) {
        res &= f(*begin);
    }
    return res;
}

template <class T, class UnaryPredicate>
bool all_in_container_match_predicate (const T &container, UnaryPredicate f) {
    return all_match_predicate(container.begin(), container.end(), f);
}

bool notf(bool x);

/* Translates to and from `0123456789ABCDEF`. */
bool hex_to_int(char c, int *out);
char int_to_hex(int i);

std::string blocking_read_file(const char *path);
bool blocking_read_file(const char *path, std::string *contents_out);

template <class T>
class assignment_sentry_t {
public:
    assignment_sentry_t() : var(nullptr), old_value() { }
    assignment_sentry_t(T *v, const T &value) :
            var(v), old_value(*var) {
        *var = value;
    }
    ~assignment_sentry_t() {
        reset();
    }
    void reset(T *v, const T &value) {
        reset();
        var = v;
        old_value = *var;
        *var = value;
    }
    void reset() {
        if (var != nullptr) {
            *var = old_value;
            var = nullptr;
        }
    }
private:
    T *var;
    T old_value;
};

std::string errno_string(int errsv);

std::string sanitize_for_logger(const std::string &s);
static inline std::string time2str(const time_t &t) {
    const int TIMEBUF_SIZE = 26; // As specified in man 3 ctime and by MSDN
    char timebuf[TIMEBUF_SIZE];
#ifdef _WIN32
    errno_t ret = ctime_s(timebuf, sizeof(timebuf), &t);
    guarantee_err(ret == 0, "time2str: invalid time");
    return timebuf;
#else
    return ctime_r(&t, timebuf);
#endif
}

#define MSTR(x) stringify(x) // Stringify a macro
#if defined __clang__
#define COMPILER_STR "CLANG " __clang_version__
#elif defined __GNUC__
#define COMPILER_STR "GCC " MSTR(__GNUC__) "." MSTR(__GNUC_MINOR__) "." MSTR(__GNUC_PATCHLEVEL__)
#elif defined _MSC_VER
#define COMPILER_STR "MSC " MSTR(_MSC_FULL_VER)
#else
#define COMPILER_STR "UNKNOWN COMPILER"
#endif

#ifndef NDEBUG
#define RETHINKDB_VERSION_STR "rethinkdb " RETHINKDB_VERSION " (debug)" " (" COMPILER_STR ")"
#else
#define RETHINKDB_VERSION_STR "rethinkdb " RETHINKDB_VERSION " (" COMPILER_STR ")"
#endif

#define ANY_PORT 0

template <class T>
T clone(const T& x) {
    return x;
}

#endif // UTILS_HPP_
