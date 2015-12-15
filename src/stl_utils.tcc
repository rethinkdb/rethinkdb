// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef STL_UTILS_TCC_
#define STL_UTILS_TCC_

#include "stl_utils.hpp"

#include <algorithm>
#include <deque>
#include <map>
#include <set>
#include <utility>
#include <vector>

#include "containers/printf_buffer.hpp"

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
void debug_print_iterators(printf_buffer_t *buf, It beg, It end) {
    for (It it = beg; it != end; ++it) {
        if (it != beg) {
            buf->appendf(", ");
        }
        buf->appendf("\n");
        debug_print(buf, *it);
    }
}

template <class K, class V, class C>
void debug_print(printf_buffer_t *buf, const std::map<K, V, C> &map) {
    buf->appendf("{");
    debug_print_iterators(buf, map.begin(), map.end());
    buf->appendf("}");
}

template <class T>
void debug_print(printf_buffer_t *buf, const std::set<T> &set) {
    buf->appendf("#{");
    debug_print_iterators(buf, set.begin(), set.end());
    buf->appendf("}");
}

template <class T>
void debug_print(printf_buffer_t *buf, const std::vector<T> &vec) {
    buf->appendf("[");
    debug_print_iterators(buf, vec.begin(), vec.end());
    buf->appendf("]");
}

template <class T>
void debug_print(printf_buffer_t *buf, const std::deque<T> &deque) {
    buf->appendf("deque{");
    debug_print_iterators(buf, deque.begin(), deque.end());
    buf->appendf("}");
}

template <class T, class U>
void debug_print(printf_buffer_t *buf, const std::pair<T, U> &p) {
    debug_print(buf, p.first);
    buf->appendf(" => ");
    debug_print(buf, p.second);
}

#endif  // STL_UTILS_TCC_
