// Copyright (C) 2006 Arkadiy Vertleyb
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"

#include <boost/typeof/std/string.hpp>
#include <boost/typeof/std/deque.hpp>
#include <boost/typeof/std/list.hpp>
#include <boost/typeof/std/queue.hpp>
#include <boost/typeof/std/stack.hpp>
#include <boost/typeof/std/vector.hpp>
#include <boost/typeof/std/map.hpp>
#include <boost/typeof/std/set.hpp>
#include <boost/typeof/std/bitset.hpp>
#include <boost/typeof/std/functional.hpp>
#include <boost/typeof/std/valarray.hpp>
#include <boost/typeof/std/locale.hpp>
#include <boost/typeof/std/iostream.hpp>
#include <boost/typeof/std/streambuf.hpp>
#include <boost/typeof/std/istream.hpp>
#include <boost/typeof/std/ostream.hpp>
#include <boost/typeof/std/sstream.hpp>
#include <boost/typeof/std/fstream.hpp>
#include <boost/typeof/std/iterator.hpp>

using namespace std;

// STL containers

BOOST_STATIC_ASSERT(boost::type_of::test<string>::value);
BOOST_STATIC_ASSERT(boost::type_of::test<deque<int> >::value);
BOOST_STATIC_ASSERT(boost::type_of::test<list<int> >::value);
BOOST_STATIC_ASSERT(boost::type_of::test<queue<int> >::value);
BOOST_STATIC_ASSERT(boost::type_of::test<stack<int> >::value);
BOOST_STATIC_ASSERT(boost::type_of::test<vector<int> >::value);
BOOST_STATIC_ASSERT((boost::type_of::test<map<int, int> >::value));
BOOST_STATIC_ASSERT((boost::type_of::test<multimap<int, int> >::value));
BOOST_STATIC_ASSERT(boost::type_of::test<set<int> >::value);
BOOST_STATIC_ASSERT(boost::type_of::test<multiset<int> >::value);
BOOST_STATIC_ASSERT(boost::type_of::test<bitset<10> >::value);

// function objects

BOOST_STATIC_ASSERT((boost::type_of::test<unary_function<int, int> >::value));
BOOST_STATIC_ASSERT((boost::type_of::test<binary_function<int, int, int> >::value));
BOOST_STATIC_ASSERT(boost::type_of::test<plus<int> >::value);
BOOST_STATIC_ASSERT(boost::type_of::test<minus<int> >::value);
BOOST_STATIC_ASSERT(boost::type_of::test<multiplies<int> >::value);
BOOST_STATIC_ASSERT(boost::type_of::test<divides<int> >::value);
BOOST_STATIC_ASSERT(boost::type_of::test<modulus<int> >::value);
BOOST_STATIC_ASSERT(boost::type_of::test<negate<int> >::value);
BOOST_STATIC_ASSERT(boost::type_of::test<equal_to<int> >::value);
BOOST_STATIC_ASSERT(boost::type_of::test<not_equal_to<int> >::value);
BOOST_STATIC_ASSERT(boost::type_of::test<greater<int> >::value);
BOOST_STATIC_ASSERT(boost::type_of::test<less<int> >::value);
BOOST_STATIC_ASSERT(boost::type_of::test<greater_equal<int> >::value);
BOOST_STATIC_ASSERT(boost::type_of::test<less_equal<int> >::value);
BOOST_STATIC_ASSERT(boost::type_of::test<logical_and<int> >::value);
BOOST_STATIC_ASSERT(boost::type_of::test<logical_or<int> >::value);
BOOST_STATIC_ASSERT(boost::type_of::test<logical_not<int> >::value);
BOOST_STATIC_ASSERT(boost::type_of::test<unary_negate<negate<int> > >::value);
BOOST_STATIC_ASSERT(boost::type_of::test<binary_negate<less<int> > >::value);
BOOST_STATIC_ASSERT(boost::type_of::test<binder1st<less<int> > >::value);
BOOST_STATIC_ASSERT(boost::type_of::test<binder2nd<less<int> > >::value);

// valarray

BOOST_STATIC_ASSERT(boost::type_of::test<valarray<int> >::value);
