#ifndef __UTILS2_HPP__
#define __UTILS2_HPP__

/* utils2.hpp is a collection of utility functions and types that sits below the IO layer.
The reason it is separate from utils.hpp is that the IO layer needs some of the things in
utils2.hpp, but utils.hpp needs some things in the IO layer. */

#include "config/args.hpp"
#include <stdint.h>
#include <time.h>
#include "errors.hpp"

typedef uint64_t cas_t;

int get_cpu_count();
long get_available_ram();
long get_total_ram();

// This may be surprising to some people.
typedef char byte;

// for safety  TODO: move this to a different file
struct repli_timestamp {
    uint32_t time;

    static const repli_timestamp invalid;
};

struct initialized_repli_timestamp {
    uint32_t time;
    explicit initialized_repli_timestamp(uint32_t _time) : time(_time) { }
    initialized_repli_timestamp(repli_timestamp timestamp) : time(timestamp.time) { }

    operator repli_timestamp() const {
        repli_timestamp ret = { time };
        return ret;
    }
};

struct charslice {
    char *beg, *end;
    charslice(char *beg_, char *end_) : beg(beg_), end(end_) { }
    charslice() : beg(NULL), end(NULL) { }
};

struct const_charslice {
    const char *beg, *end;
    const_charslice(const char *beg_, const char *end_) : beg(beg_), end(end_) { }
    const_charslice() : beg(NULL), end(NULL) { }
};



// Converts a time_t (in seconds) to a repli_timestamp.  The important
// thing here is that this will never return repli_timestamp::invalid,
// which will matter for one second every 116 years.
repli_timestamp repli_time(time_t t);

// TODO: move this to a different file
repli_timestamp current_time();

typedef uint64_t microtime_t;

microtime_t current_microtime();

// This is not a transitive operation.  It compares times "locally."
// Imagine a comparison function that compares angles, in the range
// [0, 2*pi), that is invariant with respect to rotation.  How would
// you implement that?  This is a function that compares timestamps in
// [0, 2**32), that is invariant with respect to translation.
int repli_compare(repli_timestamp x, repli_timestamp y);

// Like std::max, except it's technically not associative because it
// uses repli_compare.
repli_timestamp repli_max(repli_timestamp x, repli_timestamp y);


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
float ticks_to_secs(ticks_t ticks);
float ticks_to_ms(ticks_t ticks);
float ticks_to_us(ticks_t ticks);

/* Functions to create random delays. These must be in utils2.hpp instead of in
utils.hpp because the mock IO layer uses random delays. Internally, they
secretly use the IO layer, but it is safe to include utils2.hpp from within the
IO layer. */

void random_delay(void (*)(void*), void*);

template<class cb_t>
void random_delay(cb_t *cb, void (cb_t::*method)());

template<class cb_t, class arg1_t>
void random_delay(cb_t *cb, void (cb_t::*method)(arg1_t), arg1_t arg);

template<class cb_t>
bool maybe_random_delay(cb_t *cb, void (cb_t::*method)());

template<class cb_t, class arg1_t>
bool maybe_random_delay(cb_t *cb, void (cb_t::*method)(arg1_t), arg1_t arg);

/* Debugging printing API (prints current thread in addition to message) */

void debugf(const char *msg, ...);

// Returns a random number in [0, n).  Is not perfectly uniform; the
// bias tends to get worse when RAND_MAX is far from a multiple of n.
int randint(int n);

// The existence of these functions does not constitute an endorsement
// for casts.  These constitute an endorsement for the use of
// reinterpret_cast, rather than C-style casts.  The latter can break
// const correctness.
template <class T>
inline const T* ptr_cast(const void *p) { return reinterpret_cast<const T*>(p); }

template <class T>
inline T* ptr_cast(void *p) { return reinterpret_cast<T*>(p); }

bool begins_with_minus(const char *string);
// strtoul() and strtoull() will for some reason not fail if the input begins with a minus
// sign. strtoul_strict() and strtoull_strict() do.
long strtol_strict(const char *string, char **end, int base);
unsigned long strtoul_strict(const char *string, char **end, int base);
unsigned long long strtoull_strict(const char *string, char **end, int base);
bool strtobool_strict(const char *string, char **end);

// Put this in a private: section.
#define DISABLE_COPYING(T)                      \
    T(const T&);                                \
    void operator=(const T&)

// This is inefficient, it calls vsnprintf twice and copies the
// arglist and output buffer excessively.
std::string strprintf(const char *format, ...) __attribute__ ((format (printf, 1, 2)));

// Pad a value to the size of a cache line to avoid false sharing.
// TODO: This is implemented as a struct with subtraction rather than a union
// so that it gives an error when trying to pad a value bigger than
// CACHE_LINE_SIZE. If that's needed, this may have to be done differently.
// TODO: Use this in the rest of the perfmons, if it turns out to make any
// difference.

template<typename value_t>
struct cache_line_padded_t {
    value_t value;
    byte padding[CACHE_LINE_SIZE - sizeof value];
};

#include "utils2.tcc"

#endif /* __UTILS2_HPP__ */
