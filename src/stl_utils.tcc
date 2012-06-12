#ifndef STL_UTILS_TCC_
#define STL_UTILS_TCC_

#include "stl_utils.hpp"

#include <algorithm>

#include <boost/optional.hpp>

//TODO this can be much more efficient with an iterator
template <class K, class V>
std::set<K> keys(const std::map<K, V> &map) {
    std::set<K> res;
    for (typename std::map<K, V>::const_iterator it = map.begin(); it != map.end(); ++it) {
        res.insert(it->first);
    }

    return res;
}

// If keys is called "keys", this should be called "values".
template <class K, class V>
std::set<V> range(const std::map<K, V> &map) {
    std::set<V> res;

    for (typename std::map<K, V>::const_iterator it =  map.begin(); it != map.end(); ++it) {
        res.insert(it->second);
    }

    return res;
}

template <class container_t>
bool std_contains(const container_t &container, const typename container_t::key_type &key) {
    return container.find(key) != container.end();
}

template <class container_t>
bool std_does_not_contain(const container_t &container, const typename container_t::key_type &key) {
    return !std_contains<container_t>(container, key);
}

template <class container_t>
typename container_t::mapped_type &get_with_default(container_t &container, const typename container_t::key_type &key, const typename container_t::mapped_type &default_value) {
    if (container.find(key) == container.end()) {
        container[key] = default_value;
    }

    return container[key];
}

template <class left_container_t, class right_container_t>
cartesian_product_iterator_t<left_container_t, right_container_t>::cartesian_product_iterator_t(
        typename left_container_t::iterator _left, typename left_container_t::iterator _left_end,
        typename right_container_t::iterator _right, typename right_container_t::iterator _right_end)
    : left(_left), left_start(_left), left_end(_left_end),
      right(_right), right_start(_right), right_end(_right_end)
{ }

template <class left_container_t, class right_container_t>
boost::optional<std::pair<typename left_container_t::iterator, typename right_container_t::iterator> >
cartesian_product_iterator_t<left_container_t, right_container_t>::operator*() const {
    if (right != right_end) {
        return boost::make_optional(std::make_pair(left, right));
    } else {
        return boost::optional<std::pair<typename left_container_t::iterator, typename right_container_t::iterator> >();
    }
}

template <class left_container_t, class right_container_t>
cartesian_product_iterator_t<left_container_t, right_container_t>
cartesian_product_iterator_t<left_container_t, right_container_t>::operator+(int val) {
    cartesian_product_iterator_t<left_container_t, right_container_t> res = *this;
    while (val --> 0) {
        res.left++;
        if (res.left == res.left_end) {
            res.right++;
            res.left = res.left_start;
        }
    }

    return res;
}

template <class left_container_t, class right_container_t>
cartesian_product_iterator_t<left_container_t, right_container_t> &
cartesian_product_iterator_t<left_container_t, right_container_t>::operator++(int) {
    return *this = *this + 1;
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

#endif  // STL_UTILS_TCC_
