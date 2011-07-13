#include "stats/serialize.hpp"
#include "errors.hpp"

#include <limits.h>
#include <sstream>

void serialize_i64(std::ostream &s, int64_t val) { serialize_u64(s, (uint64_t) val); }
int64_t unserialize_i64(std::istream &s) { return (int64_t) unserialize_u64(s); }

void serialize_u64(std::ostream &s, uint64_t val) {
    rassert(CHAR_BIT == 8);     // sanity.
    char buf[sizeof(val)];
    // Write val big-endian into buf, from low-to-high byte
    for (int i = sizeof(buf) - 1; i >= 0; --i) {
        buf[i] = (char) (val & 0xff);
        val >>= 8;
    }

    rassert(s.good());
    s.write(buf, sizeof(buf));
    rassert(!s.fail());         // could have reached EOF
}

uint64_t unserialize_u64(std::istream &s) {
    rassert(CHAR_BIT == 8);     // sanity
    rassert(s.good());
    char buf[sizeof(uint64_t)];
    s.read(buf, sizeof(buf));
    rassert(s.good());

    // Read val big-endian from buf, from low-to-high byte
    uint64_t result = 0;
    for (int i = 0; (unsigned) i < sizeof(buf); ++i) {
        result += buf[sizeof(buf) - i - 1] << (8 * i);
    }
    return result;
}

// TODO (rntz) this is not a portable representation format
void serialize_float(std::ostream &s, float val) {
    rassert(s.good());
    s.write(reinterpret_cast<char *>(&val), sizeof(val));
    rassert(!s.fail());
}

float unserialize_float(std::istream &s) {
    float x;
    rassert(s.good());
    s.read(reinterpret_cast<char *>(&x), sizeof(x));
    rassert(!s.fail());
    return x;
}
