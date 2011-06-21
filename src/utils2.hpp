#ifndef __UTILS2_HPP__
#define __UTILS2_HPP__

/* utils2.hpp is a collection of utility functions and types that sits below the IO layer.
The reason it is separate from utils.hpp is that the IO layer needs some of the things in
utils2.hpp, but utils.hpp needs some things in the IO layer. */

#include <stdlib.h>
#include <string>
#include <stdint.h>
#include "config/args.hpp"
#include "errors.hpp"

typedef uint64_t cas_t;

/* Note that repli_timestamp_t does NOT represent an actual timestamp; instead it's an arbitrary
counter. */

// for safety
struct repli_timestamp_t {
    uint32_t time;

    bool operator==(repli_timestamp_t t) const {
        return time == t.time;
    }
    bool operator<(repli_timestamp_t t) const {
        return time < t.time;
    }
    bool operator>=(repli_timestamp_t t) const {
        return time >= t.time;
    }

    repli_timestamp_t next() const {
        repli_timestamp_t t;
        t.time = time + 1;
        return t;
    }

    static const repli_timestamp_t distant_past;
    static const repli_timestamp_t invalid;
};

// TODO: switch name over completely to "_t" version
typedef repli_timestamp_t repli_timestamp;

struct const_charslice {
    const char *beg, *end;
    const_charslice(const char *beg_, const char *end_) : beg(beg_), end(end_) { }
    const_charslice() : beg(NULL), end(NULL) { }
};

typedef uint64_t microtime_t;

microtime_t current_microtime();


// Like std::max, except it's technically not associative.
repli_timestamp_t repli_max(repli_timestamp_t x, repli_timestamp_t y);


void *malloc_aligned(size_t size, size_t alignment = 64);

template <class T1, class T2>
T1 ceil_aligned(T1 value, T2 alignment) {
    return value + alignment - (((value + alignment - 1) % alignment) + 1);
}

template <class T1, class T2>
T1 ceil_divide(T1 dividend, T2 alignment) {
    return (dividend + alignment - 1) / alignment;
}

template <class T1, class T2>
T1 floor_aligned(T1 value, T2 alignment) {
    return value - (value % alignment);
}

typedef unsigned long long ticks_t;
ticks_t secs_to_ticks(float secs);
ticks_t get_ticks();
long get_ticks_res();
double ticks_to_secs(ticks_t ticks);

// HEY: Maybe debugf and log_call and TRACEPOINT should be placed in
// debug.hpp (and debug.cc).
/* Debugging printing API (prints current thread in addition to message) */
#ifndef NDEBUG
void debugf(const char *msg, ...) __attribute__((format (printf, 1, 2)));
#else
#define debugf(...) ((void)0)
#endif

// Returns a random number in [0, n).  Is not perfectly uniform; the
// bias tends to get worse when RAND_MAX is far from a multiple of n.
int randint(int n);

bool begins_with_minus(const char *string);
// strtoul() and strtoull() will for some reason not fail if the input begins with a minus
// sign. strtoul_strict() and strtoull_strict() do.
long strtol_strict(const char *string, char **end, int base);
unsigned long strtoul_strict(const char *string, char **end, int base);
unsigned long long strtoull_strict(const char *string, char **end, int base);

// Put this in a private: section.
#define DISABLE_COPYING(T)                      \
    T(const T&);                                \
    void operator=(const T&)

// This is inefficient, it calls vsnprintf twice and copies the
// arglist and output buffer excessively.
std::string strprintf(const char *format, ...) __attribute__ ((format (printf, 1, 2)));

#endif /* __UTILS2_HPP__ */
