// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef STL_UTILS_HPP_
#define STL_UTILS_HPP_

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "errors.hpp"

class printf_buffer_t;

/* stl utils make some stl structures nicer to work with */

template <class K, class V>
std::set<K> keys(const std::map<K, V> &);

template <class container_t>
bool std_contains(const container_t &, const typename container_t::key_type &);

template <class K, class V, class C>
void debug_print(printf_buffer_t *buf, const std::map<K, V, C> &map);

template <class T>
void debug_print(printf_buffer_t *buf, const std::set<T> &map);

template <class T>
void debug_print(printf_buffer_t *buf, const std::vector<T> &vec);

template <class T, class U>
void debug_print(printf_buffer_t *buf, const std::pair<T, U> &p);

// We pass the first argument explicitly so that the compiler can infer the template parameters.
template <class T, class... Args>
std::vector<T> make_vector(const T &arg, Args... args) {
    std::vector<T> ret;
    UNUSED int dummy[] = { (ret.push_back(arg), 1), (ret.push_back(args), 1)... };
    return ret;
}

template <class K, class V, class... Args>
std::map<K, V> make_map(const std::pair<K, V> &arg, Args... args) {
    std::map<K, V> ret;
    ret.insert(arg);
    UNUSED int dummy[] = { (ret.insert(args), 1)... };
    return ret;
}

std::vector<std::string> split_string(const std::string &s, char sep);

#include "stl_utils.tcc"

#endif /* STL_UTILS_HPP_ */
