#include "math.hpp"

#include "errors.hpp"

int64_t int64_round_up_to_power_of_two(int64_t x) {
    rassert(x >= 0);

    --x;

    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;

    rassert(x < INT64_MAX);

    return x + 1;
}

uint64_t uint64_round_up_to_power_of_two(uint64_t x) {
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    return x + 1;
}
