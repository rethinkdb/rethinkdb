#include "math.hpp"

#include <math.h>

// For std.
#include <iosfwd>

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

bool risfinite(double arg) {
    // isfinite is a macro on OS X in math.h, so we can't just say std::isfinite.
    using namespace std; // NOLINT(build/namespaces) due to platform variation
    return isfinite(arg);
}

bool hex_to_int(char c, int *out) {
    if (c >= '0' && c <= '9') {
        *out = c - '0';
        return true;
    } else if (c >= 'a' && c <= 'f') {
        *out = c - 'a' + 10;
        return true;
    } else if (c >= 'A' && c <= 'F') {
        *out = c - 'A' + 10;
        return true;
    } else {
        return false;
    }
}

char int_to_hex(int x) {
    rassert(x >= 0 && x < 16);
    if (x < 10) {
        return '0' + x;
    } else {
        return 'A' + x - 10;
    }
}

