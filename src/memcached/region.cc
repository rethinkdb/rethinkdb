#include "memcached/region.hpp"

#include <algorithm>

bool region_is_superset(const key_range_t &potential_superset, const key_range_t &potential_subset) THROWS_NOTHING {
    /* Special-case empty ranges */
    if (key_range_t::right_bound_t(potential_subset.left) == potential_subset.right) return true;

    if (potential_superset.left > potential_subset.left) return false;
    if (potential_superset.right < potential_subset.right) return false;

    return true;
}

key_range_t region_intersection(const key_range_t &r1, const key_range_t &r2) THROWS_NOTHING {
    if (!region_overlaps(r1, r2)) {
        return key_range_t::empty();
    }
    key_range_t ixn;
    ixn.left = r1.left < r2.left ? r2.left : r1.left;
    ixn.right = r1.right > r2.right ? r2.right : r1.right;
    return ixn;
}

bool compare_range_by_left(const key_range_t &r1, const key_range_t &r2) {
    return r1.left < r2.left;
}

region_join_result_t region_join(const std::vector<key_range_t> &vec, key_range_t *out) THROWS_NOTHING {
    if (vec.empty()) {
        *out = key_range_t::empty();
        return REGION_JOIN_OK;
    } else {
        std::vector<key_range_t> sorted = vec;
        std::sort(sorted.begin(), sorted.end(), &compare_range_by_left);
        // TODO: Why the is this called a "cursor"?
        key_range_t::right_bound_t cursor = key_range_t::right_bound_t(sorted[0].left);
        // TODO: Avoid C-style casts.
        for (int i = 0; i < (int)sorted.size(); i++) {
            if (cursor < key_range_t::right_bound_t(sorted[i].left)) {
                return REGION_JOIN_BAD_REGION;
            } else if (cursor > key_range_t::right_bound_t(sorted[i].left)) {
                return REGION_JOIN_BAD_JOIN;
            } else {
                /* The regions match exactly; move on to the next one. */
                cursor = sorted[i].right;
            }
        }
        key_range_t key_union;
        key_union.left = sorted[0].left;
        key_union.right = cursor;
        *out = key_union;
        return REGION_JOIN_OK;
    }
}

bool region_is_empty(const key_range_t &r) {
    return key_range_t::right_bound_t(r.left) == r.right;
}

bool region_overlaps(const key_range_t &r1, const key_range_t &r2) THROWS_NOTHING {
    return (key_range_t::right_bound_t(r1.left) < r2.right &&
            key_range_t::right_bound_t(r2.left) < r1.right &&
            !region_is_empty(r1) && !region_is_empty(r2));
}

// What does minuend mean?  And I think you mean subtrahendron.
std::vector<key_range_t> region_subtract_many(key_range_t minuend, const std::vector<key_range_t>& subtrahends) {
    std::vector<key_range_t> buf, temp_result_buf;
    buf.push_back(minuend);
    for (std::vector<key_range_t>::const_iterator s = subtrahends.begin(); s != subtrahends.end(); ++s) {
        for (std::vector<key_range_t>::const_iterator m = buf.begin(); m != buf.end(); ++m) {
            // We are computing m-s here for each m in buf and s in subtrahends:
            // m-s = m & not(s) = m & ([-inf, s.left) | [s.right, +inf))
            //                  = (m & [-inf, s.left)) | (m & [s.right, +inf))
            key_range_t left = region_intersection(*m, key_range_t(key_range_t::none, store_key_t(), key_range_t::open, (*s).left));
            if (!left.is_empty()) {
                temp_result_buf.push_back(left);
            }
            if (!s->right.unbounded) {
                key_range_t right = region_intersection(*m, key_range_t(key_range_t::closed, s->right.key, key_range_t::none, store_key_t()));

                if (!right.is_empty()) {
                    temp_result_buf.push_back(right);
                }
            }
        }
        buf.swap(temp_result_buf);
        temp_result_buf.clear();
    }
    return buf;
}

