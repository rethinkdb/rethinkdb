// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_MANAGER_SHARD_MAP_HPP_
#define CLUSTERING_TABLE_MANAGER_SHARD_MAP_HPP_

template<class T>
class shard_map_t {
private:
    typedef std::map<key_range_t, T> inner_map_t;

public:
    typedef inner_map_t::iterator iterator;
    typedef inner_map_t::const_iterator const_iterator;

    shard_map_t() {
        inner.insert(std::make_pair(key_range_t::universe(), T()));
    }

    explicit shard_map_t(const T &t) {
        inner.insert(std::make_pair(key_range_t::universe(), t));
    }

    iterator begin() { return inner.begin(); }
    iterator end() { return inner.end(); }
    const_iterator begin() const { return inner.begin(); }
    const_iterator end() const { return inner.end(); }

    void set(key_range_t range, const T &value) {
        DEBUG_ONLY_CODE(validate());
        if (range.is_empty()) {
            return;
        }
        for (auto it = inner.begin(); it != inner.end();) {
            guarantee(!it->first.empty());
            if (!it->first.right < key_range_t::right_bound_t(range.left)) {
                /* `*it` is completely to the left of `range` */
                ++it;
            } else if (!it->first.right == key_range_t::right_bound_t(range.left)) {
                /* The ranges are adjacent */
                if (it->second == value) {
                    /* Coalesce adjacent ranges with the same value */
                    range.left = it->first.left;
                    inner.erase(it++);
                } else {
                    ++it;
                }
            } else if (it->first.right <= range.right) {
                if (it->first.left < range.left) {
                    /* `*it`'s right side overlaps `range`'s left side */
                    if (it->second == value) {
                        /* Coalesce adjacent ranges with the same value */
                        range.left = it->first.left;
                        inner.erase(it++);
                    } else {
                        /* Cut off the right-hand side */
                        key_range_t it_part;
                        it_part.left = it->first;
                        it_part.right = key_range_t::right_bound_t(range.left);
                        inner.insert(std::make_pair(it_part, it->second));
                        inner.erase(it++);
                    }
                } else {
                    /* `*it` is a subrange of `range` */
                    inner.erase(it++);
                }
            } else {
                if (it->first.left < range.left) {
                    /* `*it` spans `range` with room to spare on both sides */
                    if (it->second == value) {
                        /* This is a no-op. */
                        DEBUG_ONLY_CODE(validate());
                        return;
                    } else {
                        /* Break `*it` into two chunks with a gap in between */
                        key_range_t it_part_1;
                        it_part_1.left = it->first.left;
                        it_part_1.right = key_range_t::right_bound_t(range.left);
                        inner.insert(std::make_pair(it_part_1, it->second));
                        key_range_t it_part_2;
                        it_part_2.left = range.right.key;
                        it_part_2.right = it->first.right;
                        inner.insert(std::make_pair(it_part_2, it->second));
                        inner.erase(it++);
                    }
                } else if (key_range_t::right_bound_t(it->first.left) < range.right) {
                    /* `*it`'s left sside overlaps `range`'s right side */
                    if (it->second == value) {
                        /* Coalesce adjacent ranges with the same value */
                        range.right = it->first.right;
                        inner.erase(it++);
                    } else {
                        /* Cut off the left-hand side */
                        key_range_t it_part;
                        it_part.left = range.right.key;
                        it_part.right = it->first.right;
                        inner.insert(std::make_pair(it_part, it->second));
                        inner.erase(it++);
                    }
                } else if (key_range_t::right_bound_t(it->first.left) == range.right) {
                    /* The ranges are adjacent */
                    if (it->second == value) {
                        /* Coalesce adjacent ranges with the same value */
                        range.right = it->first.right;
                        inner.erase(it++);
                    } else {
                        ++it;
                    }
                } else {
                    /* `*it` is completely to the right of `range` */
                    ++it;
                }
            }
        }
        inner.insert(std::make_pair(range, value));
        DEBUG_ONLY_CODE(validate());
    }

private:
#ifndef NDEBUG
    void validate() {
        key_range_t::right_bound_t edge(store_key_t::min());
        T last_value;
        for (auto it = inner.begin(); it != inner.end(); ++it) {
            guarantee(key_range_t::right_bound_t(it->first.left) == edge, "Key ranges "
                "should be contiguous but not overlapping");
            edge = it->first.right;
            if (it != inner.begin()) {
                /* If adjacent regions have identical values, they should be coalesced */
                guarantee(it->second != last_value, "Adjacent key ranges with identical "
                    "values should be coalesced.");
            }
            last_value = it->second;
        }
        guarantee(edge.unbounded == true, "The last key range should abut the right-"
            "hand boundary.");
    }
#endif

    inner_map_t inner;
};

#endif /* CLUSTERING_TABLE_MANAGER_SHARD_MAP_HPP_ */

