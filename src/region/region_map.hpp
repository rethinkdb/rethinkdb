#ifndef REGION_REGION_MAP_HPP_
#define REGION_REGION_MAP_HPP_

#include <algorithm>
#include <utility>
#include <vector>

#include "utils.hpp"
#include "containers/archive/stl_types.hpp"
#include "containers/range_map.hpp"
#include "region/region.hpp"

/* `region_map_t` is a mapping from contiguous non-overlapping `region_t`s to `value_t`s.
It will automatically merge contiguous regions with the same value (in most cases).
Internally, it's implemented as two nested `range_map_t`s. */
template <class value_t>
class region_map_t {
private:
    typedef range_map_t<uint64_t, value_t> hash_range_map_t;
    typedef key_range_t::right_bound_t key_edge_t;
    typedef range_map_t<key_edge_t, hash_range_map_t> key_range_map_t;

public:
    typedef value_t mapped_type;

    explicit region_map_t(
            region_t r = region_t::universe(),
            value_t v = value_t()) THROWS_NOTHING :
        inner(
            key_edge_t(r.inner.left),
            r.inner.right,
            hash_range_map_t(r.beg, r.end, std::move(v))),
        hash_beg(r.beg),
        hash_end(r.end)
        { }

    static region_map_t from_unordered_fragments(
            std::vector<region_t> &&regions,
            std::vector<value_t> &&values) {
        region_t domain;
        region_join_result_t res = region_join(regions, &domain);
        guarantee(res == REGION_JOIN_OK);
        region_map_t map(domain);
        for (size_t i = 0; i < regions.size(); ++i) {
            map.update(regions[i], values[i]);
        }
        return map;
    }

    static region_map_t empty() {
        region_map_t r;
        r.inner = key_range_map_t(key_edge_t());
        return r;
    }

    region_map_t(const region_map_t &) = default;
    region_map_t(region_map_t &&) = default;
    region_map_t &operator=(const region_map_t &) = default;
    region_map_t &operator=(region_map_t &&) = default;

    region_t get_domain() const THROWS_NOTHING {
        if (inner.empty_domain()) {
            return region_t::empty();
        } else {
            region_t r;
            r.beg = hash_beg;
            r.end = hash_end;
            r.inner.left = inner.left_edge().key();
            r.inner.right = inner.right_edge();
            return r;
        }
    }

    const value_t &lookup(const store_key_t &key) const {
        uint64_t h = hash_region_hasher(key);
        rassert(h >= hash_beg);
        rassert(h < hash_end);
        return inner.lookup(key_edge_t(key)).lookup(h);
    }

    /* Calls `cb` for a set of (subregion, value) pairs that cover all of `region`. The
    signature of `cb` is:
        void cb(const region_t &, const value_t &)
    */
    template<class callable_t>
    void visit(const region_t &region, const callable_t &cb) const {
        inner.visit(key_edge_t(region.inner.left), region.inner.right,
            [&](const key_edge_t &l, const key_edge_t &r, const hash_range_map_t &hrm) {
                hrm.visit(region.beg, region.end,
                    [&](uint64_t b, uint64_t e, const value_t &value) {
                        key_range_t subrange;
                        subrange.left = l.key();
                        subrange.right = r;
                        region_t subregion(b, e, subrange);
                        cb(subregion, value);
                    });
            });
    }

    /* Derives a new `region_map_t` from the given region of this `region_map_t` by
    applying `cb` to transform each value. The signature of `cb` is:
        value2_t cb(const value_t &);
    and the return value will have type `region_map_t<value2_t>` and domain equal to
    `region`. */
    template<class callable_t>
    auto map(const region_t &region, const callable_t &cb) const
            -> region_map_t<typename std::decay<
                typename std::result_of<decltype(cb)(value_t)>::type>::type> {
        return region_map_t<typename std::decay<
                typename std::result_of<callable_t(value_t)>::type>::type>(
            inner.map(key_edge_t(region.inner.left), region.inner.right,
                [&](const hash_range_map_t &slice) {
                    return slice.map(region.beg, region.end, cb);
                }),
            region.beg,
            region.end);
    }

    /* Like `map()`, but the mapping can take into account the original location and can
    produce non-homogeneous results from a homogeneous input. The signature of `cb` is:
        region_map_t<value2_t> cb(const region_t &, const value_t &);
    and the return value will have type `region_map_t<value2_t>` and domain equal to
    `region`. */
    template<class callable_t>
    auto map_multi(const region_t &region, const callable_t &cb) const
            -> region_map_t<typename std::decay<
                typename std::result_of<callable_t(region_t, value_t)>::type>
                ::type::mapped_type> {
        typedef typename std::decay<
            typename std::result_of<callable_t(region_t, value_t)>::type>
            ::type::mapped_type result_t;
        return region_map_t<result_t>(
            inner.map_multi(key_edge_t(region.inner.left), region.inner.right,
            [&](const key_edge_t &l, const key_edge_t &r, const hash_range_map_t &hrm) {
                key_range_t sub_reg;
                sub_reg.left = l.key();
                sub_reg.right = r;
                range_map_t<key_edge_t, range_map_t<uint64_t, result_t> >
                    sub_res(l, r, range_map_t<uint64_t, result_t>(region.beg));
                hrm.visit(region.beg, region.end,
                [&](uint64_t b, uint64_t e, const value_t &value) {
                    region_t sub_sub_reg(b, e, sub_reg);
                    auto sub_sub_res = cb(sub_sub_reg, value);
                    rassert(sub_sub_res.get_domain() == sub_sub_reg);
                    sub_sub_res.inner.visit(l, r,
                    [&](const key_edge_t &l2, const key_edge_t &r2,
                            const range_map_t<uint64_t, result_t> &sub_sub_sub_res) {
                        sub_res.visit_mutable(l2, r2,
                        [&](const key_edge_t &, const key_edge_t &,
                                range_map_t<uint64_t, result_t> *sub_res_hrm) {
                            sub_res_hrm->extend_right(
                                range_map_t<uint64_t, result_t>(sub_sub_sub_res));
                        });
                    });
                });
                return sub_res;
            }),
            region.beg,
            region.end);
    }

    /* Copies a subset of this `region_map_t` into a new `region_map_t`. */
    MUST_USE region_map_t mask(const region_t &region) const {
        return region_map_t(
            inner.map(
                key_edge_t(region.inner.left),
                region.inner.right,
                [&](const hash_range_map_t &hm) {
                    return hm.mask(region.beg, region.end);
                }),
            region.beg,
            region.end);
    }

    bool operator==(const region_map_t &other) const {
        return inner == other.inner;
    }
    bool operator!=(const region_map_t &other) const {
        return inner != other.inner;
    }

    /* Overwrites part or all of this `region_map_t` with the contents of the given
    `region_map_t`. This does not change the domain of this `region_map_t`; `new_values`
    must lie entirely within the current domain. */
    void update(const region_map_t& new_values) {
        rassert(region_is_superset(get_domain(), new_values.get_domain()));
        new_values.inner.visit(
            new_values.inner.left_edge(), new_values.inner.right_edge(),
            [&](const key_edge_t &l, const key_edge_t &r, const hash_range_map_t &_new) {
                inner.visit_mutable(l, r,
                    [&](const key_edge_t &, const key_edge_t &, hash_range_map_t *cur) {
                        cur->update(hash_range_map_t(_new));
                    });
            });
    }

    /* `update()` sets the value for `r` to `v`. */
    void update(const region_t &r, const value_t &v) {
        rassert(region_is_superset(get_domain(), r));
        inner.visit_mutable(key_edge_t(r.inner.left), r.inner.right,
            [&](const key_edge_t &, const key_edge_t &, hash_range_map_t *cur) {
                cur->update(r.beg, r.end, clone(v));
            });
    }

    /* Merges two `region_map_t`s that cover the same part of the hash-space and adjacent
    ranges of the key-space. */
    void extend_keys_right(region_map_t &&new_values) {
        if (hash_end == hash_beg || inner.empty_domain()) {
            *this = std::move(new_values);
        } else {
            rassert(new_values.hash_beg == hash_beg && new_values.hash_end == hash_end);
            rassert(new_values.inner.left_edge() == inner.right_edge());
            inner.extend_right(std::move(new_values.inner));
        }
    }

    /* Applies `cb` to every value in the given region. If some sub-region lies partially
    inside and partially outside of `region`, then it will be split and `cb` will only be
    applied to the part that lies inside `region`. The signature of `cb` is:
        void cb(const region_t &, value_t *);
    */
    template<class callable_t>
    void visit_mutable(const region_t &region, const callable_t &cb) {
        inner.visit_mutable(key_edge_t(region.inner.left), region.inner.right,
            [&](const key_edge_t &l, const key_edge_t &r, hash_range_map_t *hrm) {
                hrm->visit_mutable(region.beg, region.end,
                    [&](uint64_t b, uint64_t e, value_t *value) {
                        key_range_t subrange;
                        subrange.left = l.key();
                        subrange.right = r;
                        region_t subregion(b, e, subrange);
                        cb(subregion, value);
                    });
            });
    }

private:
    template<class other_value_t>
    friend class region_map_t;

    template<class V>
    friend void debug_print(printf_buffer_t *buf, const region_map_t<V> &map);

    template<cluster_version_t W, class V>
    friend void serialize(write_message_t *wm, const region_map_t<V> &map);

    template<cluster_version_t W, class V>
    friend MUST_USE archive_result_t deserialize(read_stream_t *s, region_map_t<V> *map);

    region_map_t(key_range_map_t &&_inner, uint64_t _hash_beg, uint64_t _hash_end) :
        inner(_inner), hash_beg(_hash_beg), hash_end(_hash_end) { }

    key_range_map_t inner;

    /* All of the `hash_range_map_t`s in `inner` should begin and end at `hash_beg` and
    `hash_end`. We store them redundantly out here for easy access. */
    uint64_t hash_beg, hash_end;
};

template <class V>
void debug_print(printf_buffer_t *buf, const region_map_t<V> &map) {
    buf->appendf("rmap{");
    debug_print(buf, map.inner);
    buf->appendf("}");
}

template<cluster_version_t W, class V>
void serialize(write_message_t *wm, const region_map_t<V> &map) {
    static_assert(W == cluster_version_t::v2_3_is_latest,
        "serialize() is only supported for the latest version");
    serialize<W>(wm, map.inner);
    serialize<W>(wm, map.hash_beg);
    serialize<W>(wm, map.hash_end);
}

template<cluster_version_t W, class V>
MUST_USE archive_result_t deserialize(read_stream_t *s, region_map_t<V> *map) {
    switch (W) {
        case cluster_version_t::v2_3_is_latest:
        case cluster_version_t::v2_2:
        case cluster_version_t::v2_1: {
            archive_result_t res;
            res = deserialize<W>(s, &map->inner);
            if (bad(res)) { return res; }
            res = deserialize<W>(s, &map->hash_beg);
            if (bad(res)) { return res; }
            res = deserialize<W>(s, &map->hash_end);
            if (bad(res)) { return res; }
            return archive_result_t::SUCCESS;
        }
        case cluster_version_t::v2_0:
        case cluster_version_t::v1_16:
        case cluster_version_t::v1_15:
        case cluster_version_t::v1_14: {
            std::vector<std::pair<region_t, V> > pairs;
            archive_result_t res = deserialize<W>(s, &pairs);
            if (bad(res)) { return res; }
            std::vector<region_t> regions;
            std::vector<V> values;
            for (auto &&pair : pairs) {
                regions.push_back(pair.first);
                values.push_back(std::move(pair.second));
            }
            *map = region_map_t<V>::from_unordered_fragments(
                std::move(regions), std::move(values));
            return archive_result_t::SUCCESS;
        }
    default:
        unreachable();
    }
}

#endif  // REGION_REGION_MAP_HPP_
