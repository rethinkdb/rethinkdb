#ifndef HASH_REGION_HPP_
#define HASH_REGION_HPP_

#include <stdint.h>

#include <algorithm>

#include "utils.hpp"

// Returns a value in [0, HASH_REGION_HASH_SIZE).
const uint64_t HASH_REGION_HASH_SIZE = 1ULL << 63;
uint64_t hash_region_hasher(const uint8_t *s, size_t len);

template <class inner_region_t>
class hash_region_t {
public:
    // Produces the empty region.
    hash_region_t()
	: beg(0), end(0), inner() { }

    // For use with non-equal beg and end, non-empty inner.
    hash_region_t(uint64_t _beg, uint64_t _end, const inner_region_t &_inner)
	: beg(_beg), end(_end), inner(_inner) {
	guarantee(beg < end);
	guarantee(!region_is_empty(inner));
    }

    uint64_t beg, end;
    inner_region_t inner;
};

template <class inner_region_t>
bool region_is_empty(const hash_region_t<inner_region_t> &r) {
    return r.beg == r.end || region_is_empty(r.inner);
}

template <class inner_region_t>
bool region_is_superset(const hash_region_t<inner_region_t> &potential_superset,
			const hash_region_t<inner_region_t> &potential_subset) {
    return region_is_empty(potential_subset)
	|| (potential_superset.beg <= potential_subset.beg
	    && potential_superset.end >= potential_subset.end
	    && region_is_superset(potential_superset.inner, potential_subset.inner));
}

template <class inner_region_t>
hash_region_t<inner_region_t> region_intersection(const hash_region_t<inner_region_t> &r1,
						  const hash_region_t<inner_region_t> &r2) {
    if (r1.end <= r2.beg || r2.end <= r1.beg) {
	return hash_region_t<inner_region_t>();
    }

    inner_region_t inner_intersection = region_intersection(r1.inner, r2.inner);

    if (region_is_empty(inner_intersection)) {
	return hash_region_t<inner_region_t>();
    }

    return hash_region_t<inner_region_t>(std::max(r1.beg, r2.beg),
					 std::min(r1.end, r2.end),
					 inner_intersection);
}

template <class inner_region_t>
bool all_have_same_hash_interval(const std::vector< hash_region_t<inner_region_t> > &vec) {
    rassert(!vec.empty());

    for (int i = 1, e = vec.size(); i < e; ++i) {
	if (vec[i].beg != vec[0].beg || vec[i].end != vec[0].end) {
	    return false;
	}
    }
    return true;
}

template <class inner_region_t>
bool all_have_same_inner(const std::vector< hash_region_t<inner_region_t> > &vec) {
    rassert(!vec.empty());
    for (int i = 1, e = vec.size(); i < e; ++i) {
	if (vec[0].inner != vec[i].inner) {
	    return false;
	}
    }
    return true;
}

template <class inner_region_t>
MUST_USE region_join_result_t region_join(const std::vector< hash_region_t<inner_region_t> > &vec,
					  hash_region_t<inner_region_t> *out) {
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


    return REGION_JOIN_BAD_REGION;  // Or is it BAD_JOIN?  BAD_RECTANGLE?
}




#endif  // HASH_REGION_HPP_
