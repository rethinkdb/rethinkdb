// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CONTAINERS_COUNTED_STL_HPP_
#define CONTAINERS_COUNTED_STL_HPP_

#include <map>
#include <vector>

#include "containers/counted.hpp"

template<class value_t>
class counted_std_vector_t : public std::vector<value_t>,
                             public slow_atomic_countable_t<counted_std_vector_t<value_t> > {
public:
    counted_std_vector_t() { }
    explicit counted_std_vector_t(std::vector<value_t> &&vec)
        : std::vector<value_t>(std::move(vec)) { }
};

template<class key_t, class value_t>
class counted_std_map_t : public std::map<key_t, value_t>,
                          public slow_atomic_countable_t<counted_std_map_t<key_t, value_t> > {
public:
    counted_std_map_t() { }
    explicit counted_std_map_t(std::map<key_t, value_t> &&map)
        : std::map<key_t, value_t>(std::move(map)) { }
};

#endif  // CONTAINERS_COUNTED_STL_HPP_
