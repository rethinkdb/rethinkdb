#ifndef REGION_REGION_MAP_HPP_
#define REGION_REGION_MAP_HPP_

#include <algorithm>
#include <vector>
#include <utility>

#include "containers/archive/stl_types.hpp"
#include "containers/range_map.hpp"
#include "region/region.hpp"

/* Regions contained in region_map_t must never intersect. */
template <class value_t>
class region_map_t {
private:
    typedef range_map_t<uint64_t, value_t> hash_range_map_t;
    typedef key_range_t::right_bound_t key_edge_t;
    typedef range_map_t<key_edge_t, hash_range_map_t> key_range_map_t;

public:
    region_map_t() THROWS_NOTHING : region_map_t(region_t::universe(), value_t()) { }

    explicit region_map_t(region_t r, value_t v = value_t()) THROWS_NOTHING :
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

    MUST_USE region_map_t mask(region_t region) const {
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

    void update(const region_t &r, const value_t &v) {
        inner.visit_mutable(key_edge_t(r.inner.left), r.inner.right,
            [&](const key_edge_t &, const key_edge_t &, hash_range_map_t *cur) {
                cur->update(r.beg, r.end, value_t(v));
            });
    }

    void visit(
            const region_t &region,
            const std::function<void(const region_t &, const value_t &)> &cb) const {
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

private:
    region_map_t(key_range_map_t &&_inner, uint64_t _hash_beg, uint64_t _hash_end) :
        inner(_inner), hash_beg(_hash_beg), hash_end(_hash_end) { }

    key_range_map_t inner;
    uint64_t hash_beg, hash_end;
};

template <class V>
void debug_print(printf_buffer_t *buf, const region_map_t<V> &map) {
    buf->appendf("rmap{");
    debug_print(map.inner);
    buf->appendf("}");
}

template<cluster_version_t W, class V>
void serialize(write_message_t *wm, const region_map_t<V> &map) {
    std::vector<std::pair<region_t, V> > pairs;
    map.visit(map.get_domain(), [&](const region_t &reg, const V &val) {
        pairs.push_back(std::make_pair(reg, val));
    });
    serialize<W>(wm, pairs);
}

template<cluster_version_t W, class V>
MUST_USE archive_result_t deserialize(read_stream_t *s, region_map_t<V> *map) {
    std::vector<std::pair<region_t, V> > pairs;
    archive_result_t res = deserialize(s, &pairs);
    if (bad(res)) { return res; }
    std::vector<region_t> regions;
    std::vector<V> values;
    for (auto &&pair : pairs) {
        regions.push_back(pair.first);
        values.push_back(std::move(pair.second));
    }
    *map = region_map_t<V>::from_unordered_fragments(
        std::move(regions), std::move(values));
}

#endif  // REGION_REGION_MAP_HPP_
