#ifndef REGION_REGION_MAP_HPP_
#define REGION_REGION_MAP_HPP_

#include <algorithm>
#include <vector>
#include <utility>

#include "containers/archive/stl_types.hpp"
#include "region/region.hpp"
#include "rpc/serialize_macros.hpp"

/* Regions contained in region_map_t must never intersect. */
template <class value_t>
class region_map_t {
private:
    typedef std::vector<std::pair<region_t, value_t> > internal_vec_t;
public:
    typedef typename internal_vec_t::const_iterator const_iterator;
    typedef typename internal_vec_t::iterator iterator;

    /* I got the ypedefs like a std::map. */
    typedef region_t key_type;
    typedef value_t mapped_type;

    region_map_t() THROWS_NOTHING {
        regions_and_values.push_back(std::make_pair(region_t::universe(), value_t()));
    }

    explicit region_map_t(region_t r, value_t v = value_t()) THROWS_NOTHING {
        regions_and_values.push_back(std::make_pair(r, v));
    }

    template <class input_iterator_t>
    region_map_t(const input_iterator_t &_begin, const input_iterator_t &_end)
        : regions_and_values(_begin, _end)
    {
        DEBUG_ONLY(get_domain());
    }

    region_map_t(const region_map_t &) = default;
    region_map_t(region_map_t &&) = default;
    region_map_t &operator=(const region_map_t &) = default;
    region_map_t &operator=(region_map_t &&) = default;

    region_t get_domain() const THROWS_NOTHING {
        std::vector<region_t> regions;
        for (const_iterator it = begin(); it != end(); ++it) {
            regions.push_back(it->first);
        }
        region_t join;
        region_join_result_t join_result = region_join(regions, &join);
        guarantee(join_result == REGION_JOIN_OK);
        return join;
    }

    const_iterator cbegin() const {
        return regions_and_values.cbegin();
    }

    const_iterator cend() const {
        return regions_and_values.cend();
    }

    const_iterator begin() const { return cbegin(); }

    const_iterator end() const { return cend(); }

    iterator begin() {
        return regions_and_values.begin();
    }

    iterator end() {
        return regions_and_values.end();
    }

    MUST_USE region_map_t mask(region_t region) const {
        internal_vec_t masked_pairs;
        for (size_t i = 0; i < regions_and_values.size(); ++i) {
            region_t ixn = region_intersection(regions_and_values[i].first, region);
            if (!region_is_empty(ixn)) {
                masked_pairs.push_back(std::make_pair(ixn, regions_and_values[i].second));
            }
        }
        return region_map_t(masked_pairs.begin(), masked_pairs.end());
    }

    const value_t &get_point(const store_key_t &key) const {
        for (const auto &pair : regions_and_values) {
            if (region_contains_key(pair.first, key)) {
                return pair.second;
            }
        }
        crash("region_map_t::get_point() called with point not in domain");
    }

    // Important: 'update' assumes that new_values regions do not intersect
    void update(const region_map_t& new_values) {
        rassert(region_is_superset(get_domain(), new_values.get_domain()), "Update cannot expand the domain of a region_map.");
        std::vector<region_t> overlay_regions;
        for (const_iterator i = new_values.begin(); i != new_values.end(); ++i) {
            overlay_regions.push_back(i->first);
        }

        internal_vec_t updated_pairs;
        for (const_iterator i = begin(); i != end(); ++i) {
            region_t old = i->first;
            std::vector<region_t> old_subregions = region_subtract_many(old, overlay_regions);

            // Insert the unchanged parts of the old region into updated_pairs with the old value
            for (typename std::vector<region_t>::const_iterator j = old_subregions.begin(); j != old_subregions.end(); ++j) {
                updated_pairs.push_back(std::make_pair(*j, i->second));
            }
        }
        std::copy(new_values.begin(), new_values.end(), std::back_inserter(updated_pairs));

        regions_and_values = updated_pairs;
    }

    void concat(region_map_t &&new_values) {
        rassert(!region_overlaps(get_domain(), new_values.get_domain()));
        regions_and_values.insert(regions_and_values.end(),
            new_values.regions_and_values.begin(), new_values.regions_and_values.end());
        DEBUG_ONLY(get_domain());
    }

    void set(const region_t &r, const value_t &v) {
        update(region_map_t(r, v));
    }

    size_t size() const {
        return regions_and_values.size();
    }

    // A region map now has an order!  And it's indexable!  The order is
    // guaranteed to stay the same as long as you don't modify the map.
    // Hopefully this horribleness is only temporary.
    const std::pair<region_t, value_t> &get_nth(size_t n) const {
        return regions_and_values[n];
    }

    RDB_MAKE_ME_SERIALIZABLE_1(region_map_t, regions_and_values);

private:
    internal_vec_t regions_and_values;
};

template <class V>
void debug_print(printf_buffer_t *buf, const region_map_t<V> &map) {
    buf->appendf("rmap{");
    for (typename region_map_t<V>::const_iterator it = map.begin(); it != map.end(); ++it) {
        if (it != map.begin()) {
            buf->appendf(", ");
        }
        debug_print(buf, it->first);
        buf->appendf(" => ");
        debug_print(buf, it->second);
    }
    buf->appendf("}");
}

template <class V>
bool operator==(const region_map_t<V> &left, const region_map_t<V> &right) {
    if (left.get_domain() != right.get_domain()) {
        return false;
    }

    for (typename region_map_t<V>::const_iterator i = left.begin(); i != left.end(); ++i) {
        region_map_t<V> r = right.mask(i->first);
        for (typename region_map_t<V>::const_iterator j = r.begin(); j != r.end(); ++j) {
            if (j->second != i->second) {
                return false;
            }
        }
    }
    return true;
}

template <class P, class V>
bool operator!=(const region_map_t<V> &left, const region_map_t<V> &right) {
    return !(left == right);
}

template <class old_t, class new_t, class callable_t>
region_map_t<new_t> region_map_transform(const region_map_t<old_t> &original, const callable_t &callable) {
    std::vector<std::pair<region_t, new_t> > new_pairs;
    for (typename region_map_t<old_t>::const_iterator it =  original.begin();
         it != original.end();
         ++it) {
        new_pairs.push_back(std::pair<region_t, new_t>(
                it->first,
                callable(it->second)));
    }
    return region_map_t<new_t>(new_pairs.begin(), new_pairs.end());
}


#endif  // REGION_REGION_MAP_HPP_
