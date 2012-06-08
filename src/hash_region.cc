#include "hash_region.hpp"

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

const hash_region_t<key_range_t> double_lookup(int i, const std::vector<hash_region_t<key_range_t> > &vec) {
    rassert(0 <= i && i < int(vec.size() * 2));
    return vec[i / 2];
}

uint64_t double_hash_lookup(int i, const std::vector<hash_region_t<key_range_t> > &vec) {
    return i % 2 == 0 ? double_lookup(i, vec).beg : double_lookup(i, vec).end;
}

const store_key_t *double_key_lookup(int i, const std::vector<hash_region_t<key_range_t> > &vec) {
    const hash_region_t<key_range_t> &r = double_lookup(i, vec);

    if (i % 2 == 0) {
        return &r.inner.left;
    } else {
        if (r.inner.right.unbounded) {
            return NULL;
        } else {
            return &r.inner.key;
        }
    }
}


class hash_splitpoint_comparer_t {
public:
    hash_splitpoint_comparer_t(const std::vector< hash_region_t<key_range_t> > &vec) : vec_(vec) { }

    int compare(int i, int j) {
        uint64_t ival = double_hash_lookup(i, *vec_);
        uint64_t jval = double_hash_lookup(j, *vec_);

        return (ival > jval) - (ival < jval);
    }

private:
    const std::vector<hash_region_t<key_range_t> > *vec_;
};

class key_splitpoint_comparer_t {
public:
    key_splitpoint_comparer_t(const std::vector<hash_region_t<key_range_t> > *vec) : vec_(vec) { }

    int compare(int i, int j) {
        const store_key_t *ival = double_key_lookup(i, *vec_);
        const store_key_t *jval = double_key_lookup(j, *vec_);

        if (ival == NULL) {
            if (jval == NULL) {
                return 0;
            } else {
                return 1;
            }
        } else {
            if (jval == NULL) {
                return -1;
            } else {
                return ival->compare(*jval)
            }
        }
    }

private:
    const std::vector<hash_region_t<key_range_t> > *vec_;
};

template <class comparer_type>
class splitpoint_less_t {
public:
    splitpoint_less_t(const std::vector<hash_region_t<key_range_t> > *vec) : comparer_(vec) { }

    bool operator()(int i, int j) {
        return comparer_.compare(i, j) < 0;
    }

private:
    comparer_type comparer_;
};

template <class comparer_type>
class splitpoint_equal_t {
public:
    splitpoint_equal_t(const std::vector<hash_region_t<key_range_t> > *vec) : comparer_(vec) { }

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
    std::vector<int> hash_splitpoints(0, 2 * vec.size());
    std::vector<int> key_splitpoints(0, 2 * vec.size());
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
    std::vector<int>::iterator hash_end = std::unique(hash_beg, hash_splitpoints.end(), hash_eq);

    std::vector<int>::iterator key_beg = key_splitpoints.begin();
    std::sort(key_beg, key_splitpoints.end(), key_less);
    std::vector<int>::iterator key_end = std::unique(key_beg, key_splitpoints.end(); key_eq);

    // TODO: Finish implementing this function.
    // int width = hash_end - hash_beg - 1
    std::vector<bool>







    // Our regions are generally limited to being rectangles.  Here we
    // only support joins that join things row-wise or column-wise.
    // Either all the begs and ends are the same, or all the inners
    // are the same, of the regions in the vec.

    if (vec.empty()) {
	*out = hash_region_t<inner_region_t>();
	return REGION_JOIN_OK;
    }

    if (all_have_same_hash_interval(vec)) {
	// Try joining the inner regions.
	std::vector<inner_region_t> inners;
	inners.reserve(vec.size());

	for (int i = 0, e = vec.size(); i < e; ++i) {
	    inners.push_back(vec[i].inner);
	}

	region_join_result_t res;
	inner_region_t inner;
	res = region_join(inners, &inner);
	if (res == REGION_JOIN_OK) {
	    *out = hash_region_t<inner_region_t>(vec[0].beg, vec[0].end, inner);
	}

	return res;
    }

    if (all_have_same_inner(vec)) {
	std::vector< std::pair<uint64_t, uint64_t> > intervals;
	intervals.reserve(vec.size());

	for (int i = 0, e = vec.size(); i < e; ++i) {
	    intervals.push_back(std::pair<uint64_t, uint64_t>(vec[i].beg, vec[i].end));
	}

	std::sort(intervals.begin(), intervals.end());

	for (int i = 1, e = intervals.size(); i < e; ++i) {
	    // TODO: Why the fuck is one a "bad region" and one a "bad
	    // join"?
	    if (intervals[i - 1].second < intervals[i].first) {
		return REGION_JOIN_BAD_REGION;
	    } else if (intervals[i - 1].second > intervals[i].first) {
		return REGION_JOIN_BAD_JOIN;
	    }
	}

	*out = hash_region_t<inner_region_t>(intervals.front().first, intervals.back().second, vec[0].inner);
	return REGION_JOIN_OK;
    }

    debugf("bad region join\n");
    for (int i = 0, e = vec.size(); i < e; ++i) {
        debugf("bad region join %d/%d\n", i, e);
        debugf_print("region", vec[i]);
    }
    debugf("DONE\n");

    // TODO: Invent a new error code like REGION_JOIN_BAD_RECTANGLE.
    // I think it's always a sign of programmer error.
    rassert(false, "bad rectangle");

    return REGION_JOIN_BAD_REGION;  // Or is it BAD_JOIN?  BAD_RECTANGLE?
}
