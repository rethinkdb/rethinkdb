#ifndef HASH_REGION_HPP_
#define HASH_REGION_HPP_

// TODO: Find a good location for this file.

#include <stdint.h>

#include <algorithm>
#include <vector>

#include "rpc/serialize_macros.hpp"
#include "utils.hpp"

// Returns a value in [0, HASH_REGION_HASH_SIZE).
const uint64_t HASH_REGION_HASH_SIZE = 1ULL << 63;
uint64_t hash_region_hasher(const uint8_t *s, size_t len);

// Forms a region that shards an inner_region_t by a different
// dimension: hash values, which are computed by the function
// hash_region_hasher.  Represents a rectangle, with one range of
// coordinates represented by the inner region, the other range of
// coordinates being a range of hash values.  This doesn't really
// change _much_ about our perspective of regions, but one thing in
// particular is that every multistore should have a
// get_multistore_joined_region return value with .beg == 0, .end ==
// HASH_REGION_HASH_SIZE.
template <class inner_region_t>
class hash_region_t {
public:
    // Produces the empty region.
    hash_region_t()
	: beg(0), end(0), inner(inner_region_t::empty()) { }

    // For use with non-equal beg and end, non-empty inner.
    hash_region_t(uint64_t _beg, uint64_t _end, const inner_region_t &_inner)
	: beg(_beg), end(_end), inner(_inner) {
	guarantee(beg < end);
	guarantee(!region_is_empty(inner));
    }

    // For use with a non-empty inner, I think.
    explicit hash_region_t(const inner_region_t &_inner) : beg(0), end(HASH_REGION_HASH_SIZE), inner(_inner) {
        guarantee(!region_is_empty(inner));
    }

    static hash_region_t universe() {
        return hash_region_t(inner_region_t::universe());
    }

    static hash_region_t empty() {
        return hash_region_t();
    }

    // beg < end unless 0 == end and 0 == beg.
    uint64_t beg, end;
    inner_region_t inner;

private:
    RDB_MAKE_ME_SERIALIZABLE_3(beg, end, inner);

    // This is a copyable type.
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

    // TODO: Invent a new error code like REGION_JOIN_BAD_RECTANGLE.
    // I think it's always a sign of programmer error.
    rassert(false);

    return REGION_JOIN_BAD_REGION;  // Or is it BAD_JOIN?  BAD_RECTANGLE?
}

template <class inner_region_t>
bool region_overlaps(const hash_region_t<inner_region_t> &r1, const hash_region_t<inner_region_t> &r2) {
    return r1.beg < r2.end && r2.beg < r1.end
	&& region_overlaps(r1.inner, r2.inner);
}

// TODO: Implement region_subtract_many?  Naah fuck dat shit.

template <class inner_region_t>
bool operator==(const hash_region_t<inner_region_t> &r1,
		const hash_region_t<inner_region_t> &r2) {
    return r1.beg == r2.beg && r1.end == r2.end && r1.inner == r2.inner;
}

template <class inner_region_t>
bool operator!=(const hash_region_t<inner_region_t> &r1,
		const hash_region_t<inner_region_t> &r2) {
    return !(r1 == r2);
}


#endif  // HASH_REGION_HPP_
