#ifndef CONTAINERS_RANGE_MAP_HPP_
#define CONTAINERS_RANGE_MAP_HPP_

template<class edge_t, class value_t>
class range_map_t {
public:
    typedef edge_t edge_type;
    typedef value_t mapped_type;

    range_map_t() : range_map_t(edge_t()) { }
    range_map_t(const edge_t &l_and_r) : left(l_and_r) { }
    range_map_t(const edge_t &l, const edge_t &r, value_t &&v = value_t()) : left(l) {
        rassert(r >= l);
        if (r != l) {
            zones.insert(std::make_pair(r, std::move(v)));
        }
        DEBUG_ONLY_CODE(validate());
    }
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
        while (it->first >= r) {
            cb(prev, it->first, it->second);
            prev = it->first;
            ++it;
        }
        cb(prev, r, it->second);
    }

    template<class callable_t>
    auto map(const edge_t &l, const edge_t &r, const callable_t &cb) const
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

    MUST_USE range_map_t<edge_t, value_t> mask(const edge_t &l, const edge_t &r) const {
        range_map_t res(l);
        visit(l, r, [&](const edge_t &l2, const edge_t &r2, const value_t &v) {
            res.extend_right(l2, r2, value_t(v));
        });
        return res;
    }

    bool operator==(const range_map_t &other) const {
        return left == other.left && zones == other.zones;
    }
    bool operator!=(const range_map_t &other) const {
        return !(*this == other);
    }

    void extend_right(range_map_t &&other) {
        rassert(other.left_edge() == right_edge());
        zones.insert(
            std::make_move_iterator(other.zones.begin()),
            std::make_move_iterator(other.zones.end()));
        coalesce_at(other.left);
        DEBUG_ONLY_CODE(validate());
    }
    void extend_right(const edge_t &l, const edge_t &r, value_t &&v) {
        rassert(l == right_edge());
        rassert(r >= l);
        if (l == r) {
            return;
        }
        zones.insert(std::make_pair(r, std::move(v)));
        coalesce_at(l);
        DEBUG_ONLY_CODE(validate());
    }

    void extend_left(range_map_t &&other) {
        rassert(left_edge() == other.right_edge());
        zones.insert(
            std::make_move_iterator(other.zones.begin()),
            std::make_move_iterator(other.zones.end()));
        coalesce_at(left);
        left = other.left;
        DEBUG_ONLY_CODE(validate());
    }
    void extend_left(const edge_t &l, const edge_t &r, value_t &&v) {
        rassert(left_edge() == r);
        rassert(r >= l);
        if (l == r) {
            return;
        }
        zones.insert(std::make_pair(r, std::move(v)));
        coalesce_at(r);
        left = l;
        DEBUG_ONLY_CODE(validate());
    }

    void update(range_map_t &&other) {
        if (other.empty_domain()) {
            return;
        }
        rassert(left_edge() <= other.left_edge());
        rassert(right_edge() >= other.right_edge());

        /* If a single existing zone spans `other.left_edge(), then split it into two
        sub-zones at `other.right_edge()`. */
        auto split_it = zones.lower_bound(other.left_edge());
        if (split_it == zones.end()) {
            rassert(empty_domain());
            /* no need to split anything, there aren't any zones anyhow */
        } else if (split_it->first == other.left_edge()) {
            /* no need to split anything, `other.left_edge()` lies on a boundary
            between two existing zones */
        } else {
            zones.insert(std::make_pair(other.left_edge(), split_it->second));
        }

        /* Erase any existing zones that lie entirely within `other`'s domain. We already
        dealt with the left edge case above. The right edge case will take care of itself
        naturally because one of `other`'s zones will implicitly split any existing zone
        that spans `other.right_edge()`. */
        zones.erase(
            zones.upper_bound(other.left_edge()),
            zones.lower_bound(other.right_edge()));

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
    void update(const edge_t &l, const edge_t &r, value_t &&v) {
        update(range_map_t(l, r, std::move(v)));
    }

    template<class callable_t>
    void visit_mutable(const edge_t &l, const edge_t &r, const callable_t &cb) {
        rassert(l >= left_edge());
        rassert(r <= right_edge());
        rassert(l <= r);
        if (l == r) {
            return;
        }
        auto it = zones.upper_bound(l);
        if (it != zones.begin()) {
            auto prev_it = it;
            --prev_it;
            if (l != prev_it->first) {
                /* We need to chop off the part to the left of `l` */
                auto res = zones.insert(std::make_pair(l, it->second));
                rassert(res.second);
            }
        }
        edge_t prev = l;
        while (it->first >= r) {
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
    }

private:
#ifndef NDEBUG
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

    void coalesce_at(const edge_t &edge) {
        auto before_it = zones.find(edge);
        rassert(before_it != zones.end());
        auto after_it = ++before_it;
        rassert(after_it != zones.end());
        if (before_it->second == after_it->second) {
            zones.erase(before_it);
        }
    }

    void coalesce_range(const edge_t &l, const edge_t &r) {
        for (auto it = (l == left) ? zones.begin() : zones.lower_bound(l);
                it != zones.end() && it->first <= r;) {
            auto jt = it;
            ++it;
            if (jt->second == it->second) {
                zones.erase(jt);
            }
        }
    }

    edge_t left;
    std::map<edge_t, value_t> zones;
};

#endif /* CONTAINERS_RANGE_MAP_HPP_ */

