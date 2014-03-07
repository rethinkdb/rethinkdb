
// Copyright 2008-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "../helpers/prefix.hpp"
#include <boost/unordered/unordered_set_fwd.hpp>
#include "../helpers/postfix.hpp"

struct true_type { char x[100]; };
struct false_type { char x; };

false_type is_unordered_set_impl(void*);

template <class Value, class Hash, class Pred, class Alloc>
true_type is_unordered_set_impl(
        boost::unordered_set<Value, Hash, Pred, Alloc>*);

template<typename T>
void call_swap(boost::unordered_set<T>& x,
    boost::unordered_set<T>& y)
{
    swap(x,y);
}

template<typename T>
bool call_equals(boost::unordered_set<T>& x,
    boost::unordered_set<T>& y)
{
    return x == y;
}

template<typename T>
bool call_not_equals(boost::unordered_set<T>& x,
    boost::unordered_set<T>& y)
{
    return x != y;
}

template<typename T>
void call_swap(boost::unordered_multiset<T>& x,
    boost::unordered_multiset<T>& y)
{
    swap(x,y);
}

template<typename T>
bool call_equals(boost::unordered_multiset<T>& x,
    boost::unordered_multiset<T>& y)
{
    return x == y;
}

template<typename T>
bool call_not_equals(boost::unordered_multiset<T>& x,
    boost::unordered_multiset<T>& y)
{
    return x != y;
}

#include "../helpers/test.hpp"

typedef boost::unordered_set<int> int_set;
typedef boost::unordered_multiset<int> int_multiset;

UNORDERED_AUTO_TEST(use_fwd_declared_trait_without_definition) {
    BOOST_TEST(sizeof(is_unordered_set_impl((int_set*) 0))
        == sizeof(true_type));
}

#include <boost/unordered_set.hpp>

UNORDERED_AUTO_TEST(use_fwd_declared_trait) {
    boost::unordered_set<int> x;
    BOOST_TEST(sizeof(is_unordered_set_impl(&x)) == sizeof(true_type));

    BOOST_TEST(sizeof(is_unordered_set_impl((int*) 0)) == sizeof(false_type));
}

UNORDERED_AUTO_TEST(use_set_fwd_declared_function) {
    int_set x, y;
    x.insert(1);
    y.insert(2);
    call_swap(x, y);

    BOOST_TEST(y.find(1) != y.end());
    BOOST_TEST(y.find(2) == y.end());

    BOOST_TEST(x.find(1) == x.end());
    BOOST_TEST(x.find(2) != x.end());

    BOOST_TEST(!call_equals(x, y));
    BOOST_TEST(call_not_equals(x, y));
}

UNORDERED_AUTO_TEST(use_multiset_fwd_declared_function) {
    int_multiset x, y;
    call_swap(x, y);
    BOOST_TEST(call_equals(x, y));
    BOOST_TEST(!call_not_equals(x, y));
}

RUN_TESTS()
