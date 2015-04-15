// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef REGION_HASH_REGION_HPP_
#define REGION_HASH_REGION_HPP_

#include <inttypes.h>

#include <algorithm>
#include <vector>

#include "containers/archive/archive.hpp"
#include "rpc/serialize_macros.hpp"

enum region_join_result_t { REGION_JOIN_OK, REGION_JOIN_BAD_JOIN, REGION_JOIN_BAD_REGION };

struct key_range_t;
struct store_key_t;

// Returns a value in [0, HASH_REGION_HASH_SIZE).
const uint64_t HASH_REGION_HASH_SIZE = 1ULL << 63;
uint64_t hash_region_hasher(const btree_key_t *key);
uint64_t hash_region_hasher(const store_key_t &key);

// Forms a region that shards an inner_region_t by a different
// dimension: hash values, which are computed by the function
// hash_region_hasher.  Represents a rectangle, with one range of
// coordinates represented by the inner region, the other range of
// coordinates being a range of hash values.  This doesn't really
// change _much_ about our perspective of regions, but one thing in
// particular is that every multistore should have a
// get_region return value with .beg == 0, .end ==
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
    explicit hash_region_t(const inner_region_t &_inner)
        : beg(0), end(HASH_REGION_HASH_SIZE), inner(_inner) {
        guarantee(!region_is_empty(inner));
    }

    static hash_region_t universe() {
        return hash_region_t(inner_region_t::universe());
    }

    static hash_region_t empty() {
        return hash_region_t();
    }

    std::string print() {
        return strprintf("{beg: %zu, end: %zu, inner: %s}\n",
                         beg, end, inner.print().c_str());
    }

    // beg < end unless 0 == end and 0 == beg.
    uint64_t beg, end;
    inner_region_t inner;
};

// Stable serialization functions that must not change.
void serialize_for_metainfo(write_message_t *wm, const hash_region_t<key_range_t> &h);
MUST_USE archive_result_t deserialize_for_metainfo(read_stream_t *s,
                                                   hash_region_t<key_range_t> *out);

RDB_DECLARE_SERIALIZABLE(hash_region_t<key_range_t>);


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

// Uh, yeah, this key_range_t dependency here is... fine.  Whatever.  A bit
// convoluted in dependency order, we are.
MUST_USE region_join_result_t region_join(const std::vector<hash_region_t<key_range_t> > &vec,
                                          hash_region_t<key_range_t> *out);

// hash_value must equal hash_region_hasher(key.contents(), key.size()).
bool region_contains_key_with_precomputed_hash(const hash_region_t<key_range_t> &region,
                                               const store_key_t &key,
                                               uint64_t hash_value);

bool region_contains_key(const hash_region_t<key_range_t> &region, const store_key_t &key);

template <class inner_region_t>
bool region_overlaps(const hash_region_t<inner_region_t> &r1, const hash_region_t<inner_region_t> &r2) {
    return r1.beg < r2.end && r2.beg < r1.end
        && region_overlaps(r1.inner, r2.inner);
}

template <class inner_region_t>
hash_region_t<inner_region_t> drop_cpu_sharding(const hash_region_t<inner_region_t> &r) {
    return hash_region_t<inner_region_t>(0, HASH_REGION_HASH_SIZE, r.inner);
}

template <class inner_region_t>
std::vector< hash_region_t<inner_region_t> > region_subtract_many(const hash_region_t<inner_region_t> &minuend,
                                                                  const std::vector< hash_region_t<inner_region_t> > &subtrahends) {
    std::vector< hash_region_t<inner_region_t> > buf;
    std::vector< hash_region_t<inner_region_t> > temp_result_buf;

    buf.push_back(minuend);

    for (typename std::vector<hash_region_t<inner_region_t> >::const_iterator s = subtrahends.begin(); s != subtrahends.end(); ++s) {
        for (typename std::vector<hash_region_t<inner_region_t> >::const_iterator m = buf.begin(); m != buf.end(); ++m) {
            // Subtract s from m, push back onto temp_result_buf.
            // (See the "subtraction drawing" after this function.)
            // We first subtract the hash range of s from the one from m,
            // creating areas x and/or z if necessary.
            // The remaining intersection of m's and s's hash ranges is
            // then divided into sections l, r by subtracting s->inner from
            // m->inner.

            uint64_t isect_beg = std::max(m->beg, s->beg);
            uint64_t isect_end = std::min(m->end, s->end);
            // This outer conditional check is unnecessary, but it
            // might improve performance because we avoid trying an
            // unnecessary region intersection.
            if (m->beg < s->beg || s->end < m->end) {
                // Add x, if it exists.
                if (m->beg < s->beg) {
                    isect_beg = std::min(s->beg, m->end);
                    temp_result_buf.push_back(
                        hash_region_t<inner_region_t>(m->beg, isect_beg, m->inner));
                }

                // Add z, if it exists.
                if (s->end < m->end) {
                    isect_end = std::max(m->beg, s->end);
                    temp_result_buf.push_back(
                        hash_region_t<inner_region_t>(isect_end, m->end, m->inner));
                }
            }

            if (isect_beg < isect_end) {
                // The hash intersection is non-empty. Add l and r if necessary.
                const std::vector<inner_region_t> s_vec = {s->inner};
                const std::vector<inner_region_t> lr_inner =
                    region_subtract_many(m->inner, s_vec);

                for (auto it = lr_inner.begin(); it != lr_inner.end(); ++it) {
                    temp_result_buf.push_back(
                        hash_region_t<inner_region_t>(isect_beg, isect_end, *it));
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
          |      z              |
          |... ______ ..........|
          | l |  s   |     r    |
          |...|______|..........|
          |                     |
          |      x              |
   ^      |_____________________|
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
void debug_print(printf_buffer_t *buf, const hash_region_t<inner_region_t> &r) {
    buf->appendf("hash_region_t{beg: 0x%" PRIx64 ", end: 0x%" PRIx64 ", inner: ", r.beg, r.end);
    debug_print(buf, r.inner);
    buf->appendf("}");
}

#endif  // REGION_HASH_REGION_HPP_
