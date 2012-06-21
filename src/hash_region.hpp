#ifndef HASH_REGION_HPP_
#define HASH_REGION_HPP_


// TODO: Find a good location for this file.
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <algorithm>
#include <vector>

#include "rpc/serialize_macros.hpp"
#include "utils.hpp"

struct key_range_t;

// Returns a value in [0, HASH_REGION_HASH_SIZE).
const uint64_t HASH_REGION_HASH_SIZE = 1ULL << 63;
uint64_t hash_region_hasher(const uint8_t *s, ssize_t len);

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

MUST_USE region_join_result_t region_join(const std::vector< hash_region_t<key_range_t> > &vec,
					  hash_region_t<key_range_t> *out);


template <class inner_region_t>
bool region_overlaps(const hash_region_t<inner_region_t> &r1, const hash_region_t<inner_region_t> &r2) {
    return r1.beg < r2.end && r2.beg < r1.end
	&& region_overlaps(r1.inner, r2.inner);
}

template <class inner_region_t>
std::vector< hash_region_t<inner_region_t> > region_subtract_many(const hash_region_t<inner_region_t> &minuend,
                                                                  const std::vector< hash_region_t<inner_region_t> >& subtrahends) {
    std::vector< hash_region_t<inner_region_t> > buf;
    std::vector< hash_region_t<inner_region_t> > temp_result_buf;

    buf.push_back(minuend);

    for (typename std::vector< hash_region_t<inner_region_t> >::const_iterator s = subtrahends.begin(); s != subtrahends.end(); ++s) {
        for (typename std::vector< hash_region_t<inner_region_t> >::const_iterator m = buf.begin(); m != buf.end(); ++m) {
            // Subtract s from m, push back onto temp_result_buf.
            // (See the "subtraction drawing" after this function.)
            // We first subtract m.inner - s.inner, combining the
            // difference with m's hash interval to create a set of
            // regions w.  Then m.inner is intersected with s.inner,
            // and the hash range is formed from subtracting s's hash
            // range from m's, possibly creating x and/or z.

            const std::vector<inner_region_t> s_vec(1, s->inner);
            const std::vector<inner_region_t> w_inner = region_subtract_many(m->inner, s_vec);

            for (typename std::vector<inner_region_t>::const_iterator it = w_inner.begin(); it != w_inner.end(); ++it) {
                temp_result_buf.push_back(hash_region_t<inner_region_t>(m->beg, m->end, *it));
            }

            // This outer conditional check is unnecessary, but it
            // might improve performance because we avoid trying an
            // unnecessary region intersection.
            if (m->beg < s->beg || s->end < m->end) {
                inner_region_t isect = region_intersection(m->inner, s->inner);

                if (!region_is_empty(isect)) {

                    // Add x, if it exists.
                    if (m->beg < s->beg) {
                        temp_result_buf.push_back(hash_region_t<inner_region_t>(m->beg, std::min(s->beg, m->end), isect));
                    }

                    // Add z, if it exists.
                    if (s->end < m->end) {
                        temp_result_buf.push_back(hash_region_t<inner_region_t>(std::max(m->beg, s->end), m->end, isect));
                    }
                }
            }
        }

        buf.swap(temp_result_buf);
        temp_result_buf.clear();
    }

    return buf;
}

// The "subtraction drawing":
/*
           _____________________
          |   .  z   .          |
          |   .______.          |
          |w_j|  s   |    w_k   |
          |   |______|          |
          |   .      .          |
          |   .  x   .          |
   ^      |___.______.__________|
   |
   |
  hash values

     inner_region_t -->

 */


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

// Used for making std::sets of hash_region_t.
template <class inner_region_t>
bool operator<(const hash_region_t<inner_region_t> &r1, const hash_region_t<inner_region_t> &r2) {
    return (r1.beg < r2.beg || (r1.beg == r2.beg && (r1.end < r2.end || (r1.end == r2.end && r1.inner < r2.inner))));
}


template <class inner_region_t>
void debug_print(append_only_printf_buffer_t *buf, const hash_region_t<inner_region_t> &r) {
    buf->appendf("hash_region_t{beg: 0x%" PRIx64 ", end: 0x%" PRIx64 ", inner: ", r.beg, r.end);
    debug_print(buf, r.inner);
    buf->appendf("}");
}

#endif  // HASH_REGION_HPP_
