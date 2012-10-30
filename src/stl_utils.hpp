// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef STL_UTILS_HPP_
#define STL_UTILS_HPP_

#include <map>
#include <set>
#include <utility>
#include <vector>

#include "containers/printf_buffer.hpp"

/* stl utils make some stl structures nicer to work with */

template <class K, class V>
std::set<K> keys(const std::map<K, V> &);

template <class container_t>
bool std_contains(const container_t &, const typename container_t::key_type &);

template <class K, class V>
void debug_print(append_only_printf_buffer_t *buf, const std::map<K, V> &map);

template <class T>
void debug_print(append_only_printf_buffer_t *buf, const std::set<T> &map);

template <class T>
void debug_print(append_only_printf_buffer_t *buf, const std::vector<T> &vec);

template <class T, class U>
void debug_print(append_only_printf_buffer_t *buf, const std::pair<T, U> &p);

template<class A, class B>
std::map<B, A> invert_bijection_map(const std::map<A, B> &bijection);

#include "stl_utils.tcc"

#endif /* STL_UTILS_HPP_ */
