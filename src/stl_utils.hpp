#ifndef __STL_UTILS_HPP__
#define __STL_UTILS_HPP__

#include <map>

#include "errors.hpp"
#include <boost/function.hpp>
#include <boost/optional.hpp>

/* stl utils make some stl structures nicer to work with */

template <class K, class V>
std::set<K> keys(const std::map<K, V> &);

template <class K, class V>
std::set<V> range(const std::map<K, V> &);

template <class container_t>
bool std_contains(const container_t &, const typename container_t::key_type &);

template <class container_t>
bool std_does_not_contain(const container_t &, const typename container_t::key_type &);

template <class container_t>
bool std_exists_such_that(const container_t &, boost::function<bool(typename container_t::const_iterator)>);

template <class container_t>
void std_forall_pairs(const container_t &, boost::function<void(typename container_t::const_iterator, typename container_t::const_iterator)>);

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
    cartesian_product_iterator_t operator+(int);
    cartesian_product_iterator_t &operator++(int);

    bool operator==(const cartesian_product_iterator_t &);
};

#endif

#include "stl_utils.tcc"

