#include "hash_region.hpp"

#include <limits.h>

#include "btree/keys.hpp"

// TODO: Perhaps move key_range_t region stuff to btree/keys.hpp.
#include "memcached/region.hpp"

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

const hash_region_t<key_range_t> *double_lookup(int i, const std::vector<hash_region_t<key_range_t> > &vec) {
    rassert(0 <= i && i < static_cast<ssize_t>(vec.size() * 2));
    return &vec[i / 2];
}

uint64_t double_hash_lookup(int i, const std::vector<hash_region_t<key_range_t> > &vec) {
    return i % 2 == 0 ? double_lookup(i, vec)->beg : double_lookup(i, vec)->end;
}

const store_key_t *double_key_lookup(int i, const std::vector<hash_region_t<key_range_t> > &vec) {
    const hash_region_t<key_range_t> *r = double_lookup(i, vec);

    if (i % 2 == 0) {
        return &r->inner.left;
    } else {
        if (r->inner.right.unbounded) {
            return NULL;
        } else {
            return &r->inner.right.key;
        }
    }
}


class hash_splitpoint_comparer_t {
public:
    explicit hash_splitpoint_comparer_t(const std::vector< hash_region_t<key_range_t> > *vec) : vec_(vec) { }

    int compare(int i, int j) {
        uint64_t ival = double_hash_lookup(i, *vec_);
        uint64_t jval = double_hash_lookup(j, *vec_);

        int greater = ival > jval;
        int less = ival < jval;

        return greater - less;
    }

private:
public: // TODO
    const std::vector<hash_region_t<key_range_t> > *vec_;
};

class key_splitpoint_comparer_t {
public:
    explicit key_splitpoint_comparer_t(const std::vector<hash_region_t<key_range_t> > *vec) : vec_(vec) { }

    int compare(int i, int j) {
        // debugf("compare %d %d\n", i, j);

        const store_key_t *ival = double_key_lookup(i, *vec_);
        const store_key_t *jval = double_key_lookup(j, *vec_);

        // debugf_print("ihash", *double_lookup(i, *vec_));
        // debugf_print("jhash", *double_lookup(j, *vec_));
        // debugf_print("ival", ival);
        // debugf_print("jval", jval);

        int ret;
        if (ival == NULL) {
            if (jval == NULL) {
                ret = 0;
            } else {
                ret = 1;
            }
        } else {
            if (jval == NULL) {
                ret = -1;
            } else {
                ret = ival->compare(*jval);
            }
        }

        // debugf("compare returned %d\n", ret);
        return ret;
    }


private:
public:  // TODO
    const std::vector<hash_region_t<key_range_t> > *vec_;
};

template <class comparer_type>
class splitpoint_less_t {
public:
    explicit splitpoint_less_t(const std::vector<hash_region_t<key_range_t> > *vec) : comparer_(vec) { }

    bool operator()(int i, int j) {
        bool ret = comparer_.compare(i, j) < 0;
        // debugf("less %d %d -> %d\n", i, j, int(ret));
        // debugf_print("i", *double_lookup(i, *comparer_.vec_));
        // debugf_print("j", *double_lookup(j, *comparer_.vec_));
        return ret;
    }

private:
    comparer_type comparer_;
};

template <class comparer_type>
class splitpoint_equal_t {
public:
    explicit splitpoint_equal_t(const std::vector<hash_region_t<key_range_t> > *vec) : comparer_(vec) { }

    bool operator()(int i, int j) {
        return comparer_.compare(i, j) == 0;
    }
private:
    comparer_type comparer_;
};

MUST_USE region_join_result_t region_join(const std::vector< hash_region_t<key_range_t> > &vec,
                                          hash_region_t<key_range_t> *out) {

    // Look at all this allocation we're doing.  Somebody shoot me.
    // These are weird indices pointing into vec, where 0 <=
    // hash_splitpoints[i] / 2 < vec.size(), and the LSB tells whether
    // it's low or high.
    std::vector<int> hash_splitpoints(2 * vec.size());
    std::vector<int> key_splitpoints(2 * vec.size());
    for (int i = 0, e = 2 * vec.size(); i < e; ++i) {
        hash_splitpoints[i] = i;
        key_splitpoints[i] = i;
    }

    splitpoint_less_t<hash_splitpoint_comparer_t> hash_less(&vec);
    splitpoint_equal_t<hash_splitpoint_comparer_t> hash_eq(&vec);
    splitpoint_less_t<key_splitpoint_comparer_t> key_less(&vec);
    splitpoint_equal_t<key_splitpoint_comparer_t> key_eq(&vec);


    std::vector<int>::iterator hash_beg = hash_splitpoints.begin();
    std::sort(hash_beg, hash_splitpoints.end(), hash_less);

#ifdef REGION_JOIN_DEBUG
    for (size_t i = 0; i < hash_splitpoints.size(); ++i) {
        debugf("pre-unique hash_splitpoints[%zu] = %d\n", i, hash_splitpoints[i]);
        uint64_t k = double_hash_lookup(hash_splitpoints[i], vec);
        debugf_print("key", k);
    }
#endif  // REGION_JOIN_DEBUG

    std::vector<int>::iterator hash_end = std::unique(hash_beg, hash_splitpoints.end(), hash_eq);

    std::vector<int>::iterator key_beg = key_splitpoints.begin();
    std::sort(key_beg, key_splitpoints.end(), key_less);

#ifdef REGION_JOIN_DEBUG
    for (size_t i = 0; i < key_splitpoints.size(); ++i) {
        debugf("pre-unique key_splitpoints[%d] = %d\n", i, key_splitpoints[i]);
        const store_key_t *k = double_key_lookup(key_splitpoints[i], vec);
        if (k != NULL) {
            debugf_print("key", *k);
        } else {
            debugf("key: (null)\n");
        }
    }
#endif // REGION_JOIN_DEBUG


    std::vector<int>::iterator key_end = std::unique(key_beg, key_splitpoints.end(), key_eq);

#ifdef REGION_JOIN_DEBUG
    for (int i = 0; i < hash_end - hash_beg; ++i) {
        debugf("hash_splitpoints[%d] = %d\n", i, *(hash_beg + i));
        uint64_t k = double_hash_lookup(*(hash_beg + i), vec);
        debugf("key: %zx\n", k);
    }

    for (int i = 0; i < key_end - key_beg; ++i) {
        debugf("key_splitpoints[%d] = %d\n", i, *(key_beg + i));
        const store_key_t *k = double_key_lookup(*(key_beg + i), vec);
        if (k != NULL) {
            debugf_print("key", *k);
        } else {
            debugf("key: (null)\n");
        }

    }
    for (ssize_t i = key_end - key_beg; i < static_cast<ssize_t>(key_splitpoints.size()); ++i) {
        debugf("post-end key_splitpoints[%d] = %d\n", i, *(key_beg + i));
        const store_key_t *k = double_key_lookup(*(key_beg + i), vec);
        if (k != NULL) {
            debugf_print("key", *k);
        } else {
            debugf("key: (null)\n");
        }

    }
#endif  // REGION_JOIN_DEBUG


    // TODO: Finish implementing this function.
    int granular_height = hash_end - hash_beg - 1;
    int granular_width = key_end - key_beg - 1;

    if (granular_width <= 0 || granular_height <= 0) {
        guarantee(vec.size() == 0 || (vec[0].beg == 0 && vec[0].end == 0 && vec[0].inner.is_empty()));
        *out = hash_region_t<key_range_t>();
        return REGION_JOIN_OK;
    }

    guarantee(granular_width < INT_MAX / granular_height);

    std::vector<bool> covered(granular_width * granular_height, false);

    int num_covered = 0;

    for (int i = 0, e = vec.size(); i < e; ++i) {
        std::vector<int>::iterator beg_pos = std::lower_bound(hash_beg, hash_end, 2 * i, hash_less);
        std::vector<int>::iterator end_pos = std::lower_bound(beg_pos, hash_end, 2 * i + 1, hash_less);

        std::vector<int>::iterator left_pos = std::lower_bound(key_beg, key_end, 2 * i, key_less);
        std::vector<int>::iterator right_pos = std::lower_bound(left_pos, key_end, 2 * i + 1, key_less);

#ifdef REGION_JOIN_DEBUG
        debugf("beg_pos = %zd, end_pos = %zd / left_pos = %zd, right_pos = %zd\n", beg_pos - hash_beg, end_pos - hash_beg, left_pos - key_beg, right_pos - key_beg);
#endif

        for (std::vector<int>::iterator it = beg_pos; it < end_pos; ++it) {
            for (std::vector<int>::iterator jt = left_pos; jt < right_pos; ++jt) {
                int x = jt - key_beg;
                int y = it - hash_beg;
                int ix = y * granular_width + x;
#ifdef REGION_JOIN_DEBUG
                debugf("gasp i=%d x=%d y=%d ix=%d covered=%d\n", i, x, y, ix, covered[ix]);
#endif
                if (covered[ix]) {
                    return REGION_JOIN_BAD_JOIN;
                }
                covered[ix] = true;
                num_covered += 1;
            }
        }
    }

    if (num_covered < granular_width * granular_height) {
        return REGION_JOIN_BAD_REGION;
    }

    const store_key_t *out_left = double_key_lookup(*key_beg, vec);
    const store_key_t *out_right = double_key_lookup(*(key_end - 1), vec);
    uint64_t out_beg = double_hash_lookup(*hash_beg, vec);
    uint64_t out_end = double_hash_lookup(*(hash_end - 1), vec);

    rassert(out_left != NULL);

    if (out_right != NULL) {
        *out = hash_region_t<key_range_t>(out_beg, out_end, key_range_t(key_range_t::closed, *out_left, key_range_t::open, *out_right));
    } else {
        *out = hash_region_t<key_range_t>(out_beg, out_end, key_range_t(key_range_t::closed, *out_left, key_range_t::none, store_key_t()));
    }

    return REGION_JOIN_OK;
}
