#include "clustering/immediate_consistency/branch/hash_region.hpp"

// TODO: Replace this with a real hash function, if it is not one
// already.  It needs the property that values are uniformly
// distributed beteween 0 and UINT64_MAX / 2, so that splitting
// [0,UINT64_MAX/2] into equal intervals of size ((UINT64_MAX / 2 + 1)
// / n) will uniformly distribute keys.
uint64_t hash_region_hasher(const uint8_t *s, ssize_t len) {
    rassert(len >= 0);

    uint64_t h = 0x47a59e381fb2dc06ULL;
    for (ssize_t i = 0; i < len; ++i) {
	uint8_t ch = s[i];

	// By the power of magic, the 62nd, 61st, ..., 55th bits of d
	// are equal to the 0th, 1st, 2nd, ..., 7th bits of ch.  This
	// helps us meet the criterion specified above.
	uint64_t d = (((ch * 0x80200802ULL) & 0x0884422110ULL) * 0x0101010101ULL) << 23;

	h += d;
	h = h ^ (h >> 11) ^ (h << 21);
    }

    return h & 0x7fffffffffffffffULL;
    //           ^...^...^...^...
}
