#ifndef MATH_HPP_
#define MATH_HPP_

#include <math.h>
#include <stdint.h>

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

template <class T1, class T2>
T1 ceil_modulo(T1 value, T2 alignment) {
    T1 x = (value + alignment - 1) % alignment;
    return value + alignment - ((x < 0 ? x + alignment : x) + 1);
}

inline bool divides(int64_t x, int64_t y) {
    return y % x == 0;
}

int64_t round_up_to_power_of_two(int64_t x);

template <class T>
double safe_to_double(T val) {
    double res = static_cast<double>(val);
    if (val != static_cast<T>(res)) {
        return NAN;
    }
    return res;
}

#endif  // MATH_HPP_
