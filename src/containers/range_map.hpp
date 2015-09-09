#ifndef CONTAINERS_RANGE_MAP_HPP_
#define CONTAINERS_RANGE_MAP_HPP_

#include <map>

#include "debug.hpp"
#include "rpc/serialize_macros.hpp"

/* `range_map_t` maps from ranges delimited by `edge_t` to values of type `value_t`. The
ranges must be contiguous and non-overlapping; adjacent ranges with the same value will
be automatically coalesced. It can be thought of as a more efficient way of storing a
mapping from `edge_t` to `value_t`. */
template<class edge_t, class value_t>
class range_map_t {
public:
    typedef edge_t edge_type;
    typedef value_t mapped_type;

    /* Constructs an empty `range_map_t` that starts and ends at the default-constructed
    `edge_t`. */
    range_map_t() : left(edge_t()) { }

    /* Constructs an empty `range_map_t` that starts and ends at the given point. Even
    when empty, a `range_map_t` is still considered to be at some specific point in the
    `edge_t` space. */
    explicit range_map_t(const edge_t &l_and_r) : left(l_and_r) { }

    /* Constructs a `range_map_t` that stretches from `l` to `r` and has value `v` within
    that range. */
    range_map_t(const edge_t &l, const edge_t &r, value_t &&v = value_t()) : left(l) {
        rassert(r >= l);
        if (r != l) {
            zones.insert(std::make_pair(r, std::move(v)));
        }
        DEBUG_ONLY_CODE(validate());
    }

    range_map_t(range_map_t &&movee) :
            left(std::move(movee.left)),
            zones(std::move(movee.zones)) { }
    range_map_t(const range_map_t &) = default;
    range_map_t &operator=(const range_map_t &) = default;
    range_map_t &operator=(range_map_t &&) = default;

    /* Returns the left and right edges of the `range_map_t`. */
    const edge_t &left_edge() const {
        return left;
    }
    const edge_t &right_edge() const {
        if (!zones.empty()) {
            return (--zones.end())->first;
        } else {
            return left;
        }
    }

    /* Returns `true` if the `range_map_t` covers a zero-width range. This is equivalent
    to `left_edge() == right_edge()`. */
    bool empty_domain() const {
        return zones.empty();
    }

    /* Looks up the value for the location immediately to the right of `before_point`. */
    const value_t &lookup(const edge_t &before_point) const {
        rassert(before_point >= left_edge());
        rassert(before_point < right_edge());
        auto it = zones.upper_bound(before_point);
        return it->second;
    }

    /* Calls the given callback for every sub-range from `l` to `r`. If `l` or `r` lie
    within existing sub-ranges, the sub-ranges will be effectively split. The callback is
    guaranteed to be called in order. The signature of `cb` is:
        void cb(const edge_t &l, const edge_t &r, const value_t &v);
    */
    template<class callable_t>
    void visit(const edge_t &l, const edge_t &r, const callable_t &cb) const {
        rassert(l >= left_edge());
        rassert(r <= right_edge());
        rassert(l <= r);
        if (l == r) {
            return;
        }
        auto it = zones.upper_bound(l);
        edge_t prev = l;
        while (it->first < r) {
            cb(prev, it->first, it->second);
            prev = it->first;
            ++it;
        }
        cb(prev, r, it->second);
    }

    /* Derives a new `range_map_t` from some sub-range of this one by applying a function
    to every value. The signature of `cb` is:
        value2_t cb(const value_t &);
    and the return value will have type `range_map_t<edge_t, value2_t>`*/
    template<class callable_t>
    auto map(const edge_t &l, const edge_t &r, const callable_t &cb) const
            /* We need the `std::decay` here because if `callable_t` is a lambda, it's
            likely that it will return a `const T&` when what the user expects a return
            value of type `range_map_t<edge_t, T>`. */
            -> range_map_t<edge_t, typename std::decay<
                typename std::result_of<callable_t(value_t)>::type>::type> {
        typedef typename std::decay<
            typename std::result_of<callable_t(value_t)>::type>::type result_t;
        range_map_t<edge_t, result_t> res(l);
        visit(l, r, [&](const edge_t &l2, const edge_t &r2, const value_t &v) {
            res.extend_right(l2, r2, result_t(cb(v)));
        });
        return res;
    }

    /* Derives a new `range_map_t` from some sub-range of this one by applying a function
    to every value. Unlike in `map()` above, the function may depend on the location in
    the range, and it may map a homogeneous range in the input map to a non-homogeneous
    range in the output map. So the signature of `cb` is different:
        range_map_t<edge_t, value2_t> cb(
            const edge_t &l, const edge_t &r, const value_t &v);
    and the return value will have type `range_map_t<edge_t, value2_t>`. */
    template<class callable_t>
    auto map_multi(const edge_t &l, const edge_t &r, const callable_t &cb) const
            -> typename std::decay<
                typename std::result_of<callable_t(edge_t, edge_t, value_t)>::type>
                ::type {
        typedef typename std::decay<
            typename std::result_of<callable_t(edge_t, edge_t, value_t)>::type>
            ::type::mapped_type result_t;
        range_map_t<edge_t, result_t> res(l);
        visit(l, r, [&](const edge_t &l2, const edge_t &r2, const value_t &v) {
            range_map_t<edge_t, result_t> frags = cb(l2, r2, v);
            rassert(frags.left_edge() == l2);
            rassert(frags.right_edge() == r2);
            res.extend_right(std::move(frags));
        });
        return res;
    }

    /* Copies out a sub-range of this `range_map_t` as a separate `range_map_t`. */
    MUST_USE range_map_t<edge_t, value_t> mask(const edge_t &l, const edge_t &r) const {
        range_map_t res(l);
        visit(l, r, [&](const edge_t &l2, const edge_t &r2, const value_t &v) {
            res.extend_right(l2, r2, make_copy(v));
        });
        return res;
    }

    bool operator==(const range_map_t &other) const {
        return left == other.left && zones == other.zones;
    }
    bool operator!=(const range_map_t &other) const {
        return !(*this == other);
    }

    /* Extends the domain of this `range_map_t` on the right-hand side, filling the space
    with the contents of `other`. `other`'s left edge must be our previous right edge;
    our new right edge will be `other`'s right edge. */
    void extend_right(range_map_t &&other) {
        rassert(other.left_edge() == right_edge());
        if (other.empty_domain()) {
            return;
        }
        bool empty_before = empty_domain();
        zones.insert(
            std::make_move_iterator(other.zones.begin()),
            std::make_move_iterator(other.zones.end()));
        if (!empty_before) {
            coalesce_at(other.left);
        }
        DEBUG_ONLY_CODE(validate());
    }

    /* Extends the domain of this `range_map_t` on the right-hand side, filling the space
    with `v`. `l` must be our old right edge, and our new right edge will be `r`. */
    void extend_right(const edge_t &l, const edge_t &r, value_t &&v) {
        rassert(l == right_edge());
        rassert(r >= l);
        if (l == r) {
            return;
        }
        bool empty_before = empty_domain();
        zones.insert(std::make_pair(r, std::move(v)));
        if (!empty_before) {
            coalesce_at(l);
        }
        DEBUG_ONLY_CODE(validate());
    }

    /* `extend_left()` is just like `extend_right()` except that it extends the left-hand
    side instead of the right-hand side. */
    void extend_left(range_map_t &&other) {
        rassert(left_edge() == other.right_edge());
        if (other.empty_domain()) {
            return;
        }
        bool empty_before = empty_domain();
        zones.insert(
            std::make_move_iterator(other.zones.begin()),
            std::make_move_iterator(other.zones.end()));
        if (!empty_before) {
            coalesce_at(left);
        }
        left = other.left;
        DEBUG_ONLY_CODE(validate());
    }
    void extend_left(const edge_t &l, const edge_t &r, value_t &&v) {
        rassert(left_edge() == r);
        rassert(r >= l);
        if (l == r) {
            return;
        }
        bool empty_before = empty_domain();
        zones.insert(std::make_pair(r, std::move(v)));
        if (!empty_before) {
            coalesce_at(r);
        }
        left = l;
        DEBUG_ONLY_CODE(validate());
    }

    /* Overwrites part or all of the contents of this `range_map_t` with the contents of
    `other`. Does not change the domain. */
    void update(range_map_t &&other) {
        if (other.empty_domain()) {
            return;
        }
        rassert(left_edge() <= other.left_edge());
        rassert(right_edge() >= other.right_edge());

        /* If a single existing zone spans `other.left_edge(), then split it into two
        sub-zones at `other.right_edge()`. */
        if (other.left_edge() != left) {
            auto split_it = zones.lower_bound(other.left_edge());
            rassert(split_it != zones.end());
            if (split_it->first == other.left_edge()) {
                /* no need to split anything, `other.left_edge()` lies on a boundary
                between two existing zones */
            } else {
                zones.insert(std::make_pair(other.left_edge(), split_it->second));
            }
        } else {
            /* no need to split anything, `other.left_edge()` lies on the left edge of
            our leftmost zone */
        }

        /* Erase any existing zones that lie entirely within `other`'s domain. We already
        dealt with the left edge case above. The right edge case will take care of itself
        naturally because one of `other`'s zones will implicitly split any existing zone
        that spans `other.right_edge()`. */
        auto end = zones.lower_bound(other.right_edge());
        if (end->first == other.right_edge()) {
            ++end;
        }
        zones.erase(zones.upper_bound(other.left_edge()), end);

        /* Move all the zones from `other` into us */
        zones.insert(
            std::make_move_iterator(other.zones.begin()),
            std::make_move_iterator(other.zones.end()));

        /* Coalesce adjacent zones if appropriate */
        if (left_edge() != other.left_edge()) {
            coalesce_at(other.left_edge());
        }
        if (right_edge() != other.right_edge()) {
            coalesce_at(other.right_edge());
        }

        DEBUG_ONLY_CODE(validate());
    }

    /* Sets the value of the `range_map_t` between `l` and `r` to `v`. */
    void update(const edge_t &l, const edge_t &r, value_t &&v) {
        update(range_map_t(l, r, std::move(v)));
    }

    /* Calls `cb` on each sub-range between `l` and `r` with a mutable pointer to the
    value of that sub-range, which `cb` may modify. If `l` or `r` lies within a
    sub-range, that sub-range will be split. The signature of `cb` is:
        void cb(const edge_t &l, const edge_t &r, value_t *);
    For example, `update(l, r, v)` is equivalent to `visit_mutable(l, r, cb)` where `cb`
    is `[&](value_t *vp) { vp = v; }`. But `visit_mutable()` can also be used for more
    complex operations. */
    template<class callable_t>
    void visit_mutable(const edge_t &l, const edge_t &r, const callable_t &cb) {
        rassert(l >= left_edge());
        rassert(r <= right_edge());
        rassert(l <= r);
        if (l == r) {
            return;
        }
        auto it = zones.upper_bound(l);
        if (l != left && zones.count(l) == 0) {
            /* We need to chop off the part to the left of `l` */
            DEBUG_VAR auto res = zones.insert(std::make_pair(l, it->second));
            rassert(res.second);
        }
        edge_t prev = l;
        while (it->first < r) {
            cb(prev, it->first, &it->second);
            prev = it->first;
            ++it;
        }
        if (it->first != r) {
            /* We need to chop off the part to the right of `r` */
            auto res = zones.insert(std::make_pair(r, it->second));
            rassert(res.second);
            it = res.first;
        }
        rassert(it->first == r);
        cb(prev, r, &it->second);
        coalesce_range(l, r);
        DEBUG_ONLY_CODE(validate());
    }

    RDB_MAKE_ME_SERIALIZABLE_2(range_map_t, left, zones);

private:
#ifndef NDEBUG
    /* Make sure that adjacent zones have all been coalesced and that no zones are to the
    left of `left`. */
    void validate() const {
        if (!zones.empty()) {
            rassert(left < zones.begin()->first);
            for (auto it = zones.begin();;) {
                auto jt = it;
                ++it;
                if (it == zones.end()) {
                    break;
                }
                /* Use `!(x == y)` instead of `x != y` because sometimes people are lazy
                and don't define `!=` */
                rassert(!(it->second == jt->second),
                    "adjacent equal values should have been coalesced");
            }
        }
    }
#endif /* NDEBUG */

    /* Checks if the sub-ranges to the left and right of `edge` have the same value, and
    merges them if they do. `edge` must correspond to an internal boundary between two
    sub-ranges. */
    void coalesce_at(const edge_t &edge) {
        auto before_it = zones.find(edge);
        rassert(before_it != zones.end());
        auto after_it = before_it;
        ++after_it;
        rassert(after_it != zones.end());
        if (before_it->second == after_it->second) {
            zones.erase(before_it);
        }
    }

    /* Equivalent to calling `coalesce_at()` for every internal boundary from `l` to `r`,
    inclusive. `l` and `r` must be boundaries of sub-ranges, but they don't necessarily
    have to be internal boundaries; they can be the overall left and right edges. */
    void coalesce_range(const edge_t &l, const edge_t &r) {
        auto it = (l == left) ? zones.begin() : zones.find(l);
        while (it != zones.end() && it->first <= r) {
            auto jt = it;
            ++it;
            if (it == zones.end()) {
                break;
            }
            if (jt->second == it->second) {
                zones.erase(jt);
            }
        }
    }

    /* Each sub-range corresponds to an entry in `zones`. The entry's key is the
    right-hand edge of the zone; the zone's left-hand edge is determined by the previous
    entry's key. The first entry's left-hand edge is stored in `left`. Every method
    coalesces adjacent zones before it returns. Sub-ranges always have non-zero width; if
    the map has a zero-width domain then `zones` won't have any entries. */
    edge_t left;
    std::map<edge_t, value_t> zones;
};

template<class E, class V>
void debug_print(printf_buffer_t *buf, const range_map_t<E, V> &map) {
    buf->appendf("| ");
    debug_print(buf, map.left_edge());
    map.visit(map.left_edge(), map.right_edge(),
    [&](const E &, const E &r, const V &v) {
        buf->appendf(" {");
        debug_print(buf, v);
        buf->appendf("} ");
        debug_print(buf, r);
    });
    buf->appendf(" |");
}

#endif /* CONTAINERS_RANGE_MAP_HPP_ */

