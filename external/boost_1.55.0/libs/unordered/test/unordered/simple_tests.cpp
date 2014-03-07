
// Copyright 2006-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This test checks the runtime requirements of containers.

#include "../helpers/prefix.hpp"
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include "../helpers/postfix.hpp"

#include "../helpers/test.hpp"
#include <cstdlib>
#include <algorithm>
#include "../helpers/equivalent.hpp"

template <class X>
void simple_test(X const& a)
{
    test::unordered_equivalence_tester<X> equivalent(a);

    {
        X u;
        BOOST_TEST(u.size() == 0);
        BOOST_TEST(X().size() == 0);
    }

    {
        BOOST_TEST(equivalent(X(a)));
    }

    {
        X u(a);
        BOOST_TEST(equivalent(u));
    }

    {
        X u = a;
        BOOST_TEST(equivalent(u));
    }

    {
        X b(a);
        BOOST_TEST(b.begin() == const_cast<X const&>(b).cbegin());
        BOOST_TEST(b.end() == const_cast<X const&>(b).cend());
    }

    {
        X b(a);
        X c;
        BOOST_TEST(equivalent(b));
        BOOST_TEST(c.empty());
        b.swap(c);
        BOOST_TEST(b.empty());
        BOOST_TEST(equivalent(c));
        b.swap(c);
        BOOST_TEST(c.empty());
        BOOST_TEST(equivalent(b));
    }

    {
        X u;
        X& r = u;
        BOOST_TEST(&(r = r) == &r);

        BOOST_TEST(r.empty());
        BOOST_TEST(&(r = a) == &r);
        BOOST_TEST(equivalent(r));
        BOOST_TEST(&(r = r) == &r);
        BOOST_TEST(equivalent(r));
    }

    {
        BOOST_TEST(a.size() ==
            static_cast<BOOST_DEDUCED_TYPENAME X::size_type>(
                std::distance(a.begin(), a.end())));
    }

    {
        BOOST_TEST(a.empty() == (a.size() == 0));
    }

    {
        BOOST_TEST(a.empty() == (a.begin() == a.end()));
        X u;
        BOOST_TEST(u.begin() == u.end());
    }
}

UNORDERED_AUTO_TEST(simple_tests)
{
    using namespace std;
    srand(14878);

    std::cout<<"Test unordered_set.\n";
    boost::unordered_set<int> set;
    simple_test(set);

    set.insert(1); set.insert(2); set.insert(1456);
    simple_test(set);

    std::cout<<"Test unordered_multiset.\n";
    boost::unordered_multiset<int> multiset;
    simple_test(multiset);
    
    for(int i1 = 0; i1 < 1000; ++i1) {
        int count = rand() % 10, index = rand();
        for(int j = 0; j < count; ++j)
            multiset.insert(index);
    }
    simple_test(multiset);
    
    std::cout<<"Test unordered_map.\n";
    boost::unordered_map<int, int> map;

    for(int i2 = 0; i2 < 1000; ++i2) {
        map.insert(std::pair<const int, int>(rand(), rand()));
    }
    simple_test(map);

    std::cout<<"Test unordered_multimap.\n";
    boost::unordered_multimap<int, int> multimap;

    for(int i3 = 0; i3 < 1000; ++i3) {
        int count = rand() % 10, index = rand();
        for(int j = 0; j < count; ++j)
            multimap.insert(std::pair<const int, int>(index, rand()));
    }
    simple_test(multimap);
}

RUN_TESTS()
