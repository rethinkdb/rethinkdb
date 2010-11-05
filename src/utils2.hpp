#ifndef __UTILS2_HPP__
#define __UTILS2_HPP__

/* utils2.hpp is a collection of utility functions and types that sits below the IO layer.
The reason it is separate from utils.hpp is that the IO layer needs some of the things in
utils2.hpp, but utils.hpp needs some things in the IO layer. */

#include <stdint.h>
#include "errors.hpp"

int get_cpu_count();
long get_available_ram();
long get_total_ram();

typedef char byte;
typedef char byte_t;

template<typename T1, typename T2>
T1 ceil_aligned(T1 value, T2 alignment) {
    if(value % alignment != 0) {
        return value + alignment - (value % alignment);
    } else {
        return value;
    }
}

template<typename T1, typename T2>
T1 floor_aligned(T1 value, T2 alignment) {
    return value - (value % alignment);
}

template<typename T1, typename T2>
T1 ceil_aligned(T1 value, T2 alignment) {
    return value + alignment - ((value + alignment - 1) % alignment + 1);
}

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

/* Debugging printing API (prints CPU core in addition to message) */

void debugf(const char *msg, ...);

// Returns a random number in [0, n).  Is not perfectly uniform; the
// bias tends to get worse when RAND_MAX is far from a multiple of n.
int randint(int n);

// Put this in a private: declaration.
#define DISABLE_COPYING(T)                      \
    T(const T&);                                \
    void operator=(const T&)


#include "utils2.tcc"

#endif /* __UTILS2_HPP__ */
