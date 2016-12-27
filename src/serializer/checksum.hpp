#ifndef SERIALIZER_CHECKSUM_HPP_
#define SERIALIZER_CHECKSUM_HPP_

#include <stddef.h>
#include <stdint.h>

#include "arch/compiler.hpp"

// A valid checksum can never be zero, or have a zero-valued 32-bit word.  If value &
// 0xFFFFFFFFul is zero, that means this holds no checksum.  The upper 32-bit word could
// then be used to hold extra information (like whether the block has been fdatasynced).
// This defines the disk format!  Do not change.
ATTR_PACKED(struct serializer_checksum {
    uint64_t value;
    static const size_t word_size = sizeof(uint32_t);
});

// Computes a checksum of a buffer of length 4*wordcount.
// word32s: a pointer to the buffer
// wordcount: the number of 32-bit words in the buffer.
// return value: the checksum.
// The checksum is never zero.
serializer_checksum compute_checksum(const void *word32s, size_t wordcount);

// Combines checksums into the checksum of the concatenated buffer.  Given two buffers,
// s, and t, serializer_checksum_concat(serializer_checksum(s), serializer_checksum(t),
// t.wordcount) computes serializer_checksum(concat(s, t)).
serializer_checksum compute_checksum_concat(serializer_checksum left,
                                            serializer_checksum right,
                                            uint64_t right_wordcount);

// The checksum of an empty buffer.
inline serializer_checksum identity_checksum() {
    char buf[1];
    return compute_checksum(&buf, 0);
}

inline serializer_checksum no_checksum() {
    return serializer_checksum{0};
}

inline bool has_checksum(serializer_checksum x) {
    return (x.value & 0xFFFFFFFFull) != 0;
}

inline serializer_checksum datasync_checksum() {
    return serializer_checksum{0x100000000ull};
}

inline bool is_datasync_checksum(serializer_checksum x) {
    return x.value == 0x100000000ull;
}

#endif  // SERIALIZER_CHECKSUM_HPP_
