#include "serializer/checksum.hpp"

// The return value of this function or its behavior can't be changed -- the on-disk
// format obviously requires a specific checksum algorithm.
serializer_checksum compute_checksum(const void *word32s, size_t wordcount) {
    const uint32_t *p = static_cast<const uint32_t *>(word32s);

    // This is the Fletcher-64 algorithm, applied to the input whose words are xored with
    // 1.

    // Given a sequence of 32-bit words x0, x1, ..., xN-1, define sK by
    // sK = x0 + x1 + ... + xK.  Our checksum computes two values:
    // A = (sN-1 MOD (2**32 - 1)).
    // B = ((s0 + s1 + ... + sN-1) MOD (2**32 - 1)).
    //
    // We output the pair ((A == 0 ? (2**32-1) : A), (B == 0 ? (2**32-1) : B)).

    // Why Fletcher-64 on xored inputs?
    //
    // One advantage is that checksum(s + t) can be computed from checksum(s) => (A1, B1)
    // and checksum(t) => (A2, B2).  Output (A3, B3) where A3 = A1 + A2, B3 = B1 + A1 *
    // length(t) + B2.
    //
    // We first xor the words by 1, so that the word 0x00000000 is distinguishable from
    // 0xFFFFFFFF.

    uint64_t a = 0xFFFFFFFF;
    uint64_t b = 0xFFFFFFFF;

    // We go through a minor shenanigan here to handle very large buffers.
    for (;;) {
        // 0xFFFFul is low enough that a and b can't overflow.
        const size_t n = wordcount & 0xFFFFul;

        // At this point, a and b are <= 0x1_FFFF_FFFE and non-zero.

        const uint32_t xorer = 1;

        for (size_t i = 0; i < n; i++) {
            a += (uint64_t)(p[i] ^ xorer);
            b += a;
        }

        a = (a & 0xFFFFFFFFul) + (a >> 32);
        b = (b & 0xFFFFFFFFul) + (b >> 32);

        // At this point, a and b are <= 0x1_FFFF_FFFE and non-zero.

        wordcount -= n;
        if (wordcount == 0) {
            break;
        }
        p += n;
    }

    // At this point, a and b are <= 0x1_FFFF_FFFE and non-zero.
    a = (a & 0xFFFFFFFFul) + (a >> 32);
    b = (b & 0xFFFFFFFFul) + (b >> 32);
    // Now a and b are <= 0xFFFF_FFFF and non-zero.
    return serializer_checksum{(b << 32) | a};
}

serializer_checksum compute_checksum_concat(serializer_checksum left,
                                            serializer_checksum right,
                                            uint64_t right_wordcount) {
    uint64_t a_left = left.value & 0xFFFFFFFFull;
    uint64_t b_left = left.value >> 32;
    uint64_t a_right = right.value & 0xFFFFFFFFull;
    uint64_t b_right = right.value >> 32;

    // a <= 0x1_FFFF_FFFE.
    uint64_t a = a_left + a_right;
    a = (a & 0xFFFFFFFFul) + (a >> 32);
    // Now a is in the right range.

    // We have to be careful about overflow (what if right_wordcount > 0x1_0000_0000?).

    uint64_t wc = right_wordcount;
    wc = (wc & 0xFFFFFFFFul) + (wc >> 32);
    wc = (wc & 0xFFFFFFFFul) + (wc >> 32);
    // Now wc <= 0xFFFFFFFF.
    uint64_t b = a_left * right_wordcount;
    b = (b & 0xFFFFFFFFul) + (b >> 32);

    b = b + b_left + b_right;
    b = (b & 0xFFFFFFFFul) + (b >> 32);
    b = (b & 0xFFFFFFFFul) + (b >> 32);

    return serializer_checksum{(b << 32) | a};
}
