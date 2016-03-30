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

template <class T>
T clamp(T x, T lo, T hi) {
    return x < lo ? lo : x > hi ? hi : x;
}

constexpr inline bool divides(int64_t x, int64_t y) {
    return y % x == 0;
}

int64_t int64_round_up_to_power_of_two(int64_t x);
uint64_t uint64_round_up_to_power_of_two(uint64_t x);

#endif  // MATH_HPP_
