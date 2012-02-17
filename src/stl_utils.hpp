#ifndef __STL_UTILS_HPP__
#define __STL_UTILS_HPP__

#include <set>
#include <map>

//TODO this can be much more efficient with an iterator
template <class K, class V>
std::set<K> keys(const std::map<K,V> &map) {
    std::set<K> res;
    for (typename std::map<K,V>::const_iterator it = map.begin();
                                                it != map.end();
                                                it++) {
        res.insert(it->first);
    }

    return res;
}

#endif
