
#ifndef STL_UTILS_TCC_
#define STL_UTILS_TCC_

#include "stl_utils.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <utility>
#include <vector>

template <class K, class V>
std::set<K> keys(const std::map<K, V> &map) {
    std::set<K> res;
    for (typename std::map<K, V>::const_iterator it = map.begin(); it != map.end(); ++it) {
        res.insert(it->first);
    }

    return res;
}

template <class container_t>
bool std_contains(const container_t &container, const typename container_t::key_type &key) {
    return container.find(key) != container.end();
}

template <class It>
void debug_print_iterators(append_only_printf_buffer_t *buf, It beg, It end) {
    for (It it = beg; it != end; ++it) {
        if (it != beg) {
            buf->appendf(", ");
        }
        debug_print(buf, *it);
    }
}

template <class K, class V>
void debug_print(append_only_printf_buffer_t *buf, const std::map<K, V> &map) {
    buf->appendf("{");
    debug_print_iterators(buf, map.begin(), map.end());
    buf->appendf("}");
}

template <class T>
void debug_print(append_only_printf_buffer_t *buf, const std::set<T> &set) {
    buf->appendf("#{");
    debug_print_iterators(buf, set.begin(), set.end());
    buf->appendf("}");
}

template <class T>
void debug_print(append_only_printf_buffer_t *buf, const std::vector<T> &vec) {
    buf->appendf("[");
    debug_print_iterators(buf, vec.begin(), vec.end());
    buf->appendf("]");
}

template <class T, class U>
void debug_print(append_only_printf_buffer_t *buf, const std::pair<T, U> &p) {
    debug_print(buf, p.first);
    buf->appendf(" => ");
    debug_print(buf, p.second);
}

template<class A, class B>
std::map<B, A> invert_bijection_map(const std::map<A, B> &bijection) {
    std::map<B, A> inverted;
    for (typename std::map<A, B>::const_iterator it = bijection.begin(); it != bijection.end(); ++it) {
        DEBUG_VAR bool inserted = inverted.insert(std::make_pair(it->second, it->first)).second;
        rassert(inserted, "The map that was given is not a bijection and can't be inverted");
    }
    return inverted;
}

#endif  // STL_UTILS_TCC_
