#ifndef __CLUSTERING_MAP_COUNCIL_HPP__
#define __CLUSTERING_MAP_COUNCIL_HPP__

#include "rpc/council.hpp"

template<class K, class V>
class map_council_t : council_t<std::map<K,V>, std::pair<K,V> >
{
    typedef std::map<K,V> value_t;
    typedef std::pair<K,V> diff_t;
    map_council_t();
    void update(const diff_t& diff, value_t *value) {
        // for now we don't support changing values it will require attaching rwi_locks to everything
        rassert(value->find(diff.first) == value->end());
        (*value)[diff.first] = diff.second;
    }
    void insert(K k, V v) {
        apply(std::make_pair(k,v));
    }
    typename value_t::const_iterator find(K &k) const {
        return council_t<std::map<K,V>, std::pair<K,V> >::get_value().find(k);
    }

    typename value_t::const_iterator end() const {
        return council_t<std::map<K,V>, std::pair<K,V> >::get_value().end();
    }
};

#endif
