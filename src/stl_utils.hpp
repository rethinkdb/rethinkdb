#ifndef STL_UTILS_HPP_
#define STL_UTILS_HPP_

#include <map>
#include <set>

#include "errors.hpp"
#include <boost/optional.hpp>

#include "containers/printf_buffer.hpp"

/* stl utils make some stl structures nicer to work with */

template <class K, class V>
std::set<K> keys(const std::map<K, V> &);

template <class K, class V>
std::set<V> range(const std::map<K, V> &);

template <class container_t>
bool std_contains(const container_t &, const typename container_t::key_type &);

template <class container_t>
bool std_does_not_contain(const container_t &, const typename container_t::key_type &);

//Note this function is kind of a hack because if you try to access with a
//default value it will insert that value.
template <class container_t>
typename container_t::mapped_type &get_with_default(container_t &, const typename container_t::key_type &, const typename container_t::mapped_type &);

template <class left_container_t, class right_container_t>
class cartesian_product_iterator_t {
private:
    typename left_container_t::iterator left, left_start, left_end;
    typename right_container_t::iterator right, right_start, right_end;
public:
    cartesian_product_iterator_t(typename left_container_t::iterator _left, typename left_container_t::iterator _left_end,
                                 typename right_container_t::iterator _right, typename right_container_t::iterator _right_end);
    boost::optional<std::pair<typename left_container_t::iterator, typename right_container_t::iterator> > operator*() const;
    void increment();

    bool operator==(const cartesian_product_iterator_t &);
};

template <class K, class V>
void debug_print(append_only_printf_buffer_t *buf, const std::map<K, V> &map);

template <class T>
void debug_print(append_only_printf_buffer_t *buf, const std::vector<T> &vec);

template <class T, class U>
void debug_print(append_only_printf_buffer_t *buf, const std::pair<T, U> &p);


#include "stl_utils.tcc"

#endif /* STL_UTILS_HPP_ */
