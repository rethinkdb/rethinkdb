
// Copyright 2008-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "../helpers/prefix.hpp"
#include <boost/unordered/unordered_map_fwd.hpp>
#include "../helpers/postfix.hpp"

template <typename T>
void call_swap(boost::unordered_map<T,T>& x,
    boost::unordered_map<T,T>& y)
{
    swap(x,y);
}

template <typename T>
bool call_equals(boost::unordered_map<T,T>& x,
    boost::unordered_map<T,T>& y)
{
    return x == y;
}

template <typename T>
bool call_not_equals(boost::unordered_map<T,T>& x,
    boost::unordered_map<T,T>& y)
{
    return x != y;
}

template <typename T>
void call_swap(boost::unordered_multimap<T,T>& x,
    boost::unordered_multimap<T,T>& y)
{
    swap(x,y);
}

template <typename T>
bool call_equals(boost::unordered_multimap<T,T>& x,
    boost::unordered_multimap<T,T>& y)
{
    return x == y;
}

template <typename T>
bool call_not_equals(boost::unordered_multimap<T,T>& x,
    boost::unordered_multimap<T,T>& y)
{
    return x != y;
}

#include <boost/unordered_map.hpp>
#include "../helpers/test.hpp"

typedef boost::unordered_map<int, int> int_map;
typedef boost::unordered_multimap<int, int> int_multimap;

UNORDERED_AUTO_TEST(use_map_fwd_declared_function) {
    int_map x, y;
    x[1] = 2;
    y[2] = 1;
    call_swap(x, y);

    BOOST_TEST(y.find(1) != y.end() && y.find(1)->second == 2);
    BOOST_TEST(y.find(2) == y.end());

    BOOST_TEST(x.find(1) == x.end());
    BOOST_TEST(x.find(2) != x.end() && x.find(2)->second == 1);

    BOOST_TEST(!call_equals(x, y));
    BOOST_TEST(call_not_equals(x, y));
}

UNORDERED_AUTO_TEST(use_multimap_fwd_declared_function) {
    int_multimap x, y;
    call_swap(x, y);
    BOOST_TEST(call_equals(x, y));
    BOOST_TEST(!call_not_equals(x, y));
}

RUN_TESTS()
