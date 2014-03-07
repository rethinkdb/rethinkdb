
// Copyright 2006-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "../helpers/prefix.hpp"
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include "../helpers/postfix.hpp"

#include "../helpers/test.hpp"
#include <boost/next_prior.hpp>
#include "../objects/test.hpp"
#include "../helpers/random_values.hpp"
#include "../helpers/tracker.hpp"
#include "../helpers/equivalent.hpp"
#include "../helpers/helpers.hpp"
#include "../helpers/invariants.hpp"

#include <iostream>

namespace erase_tests
{

test::seed_t initialize_seed(85638);

template <class Container>
void erase_tests1(Container*, test::random_generator generator)
{
    std::cerr<<"Erase by key.\n";
    {
        test::check_instances check_;

        test::random_values<Container> v(1000, generator);
        Container x(v.begin(), v.end());
        int iterations = 0;
        for(BOOST_DEDUCED_TYPENAME test::random_values<Container>::iterator
            it = v.begin(); it != v.end(); ++it)
        {
            std::size_t count = x.count(test::get_key<Container>(*it));
            std::size_t old_size = x.size();
            BOOST_TEST(count == x.erase(test::get_key<Container>(*it)));
            BOOST_TEST(x.size() == old_size - count);
            BOOST_TEST(x.count(test::get_key<Container>(*it)) == 0);
            BOOST_TEST(x.find(test::get_key<Container>(*it)) == x.end());
            if (++iterations % 20 == 0) test::check_equivalent_keys(x);
        }
    }

    std::cerr<<"erase(begin()).\n";
    {
        test::check_instances check_;

        test::random_values<Container> v(1000, generator);
        Container x(v.begin(), v.end());
        std::size_t size = x.size();
        int iterations = 0;
        while(size > 0 && !x.empty())
        {
            BOOST_DEDUCED_TYPENAME Container::key_type
                key = test::get_key<Container>(*x.begin());
            std::size_t count = x.count(key);
            BOOST_DEDUCED_TYPENAME Container::iterator
                pos = x.erase(x.begin());
            --size;
            BOOST_TEST(pos == x.begin());
            BOOST_TEST(x.count(key) == count - 1);
            BOOST_TEST(x.size() == size);
            if (++iterations % 20 == 0) test::check_equivalent_keys(x);
        }
        BOOST_TEST(x.empty());
    }

    std::cerr<<"erase(random position).\n";
    {
        test::check_instances check_;

        test::random_values<Container> v(1000, generator);
        Container x(v.begin(), v.end());
        std::size_t size = x.size();
        int iterations = 0;
        while(size > 0 && !x.empty())
        {
            using namespace std;
            int index = rand() % (int) x.size();
            BOOST_DEDUCED_TYPENAME Container::const_iterator prev, pos, next;
            if(index == 0) {
                prev = pos = x.begin();
            }
            else {
                prev = boost::next(x.begin(), index - 1);
                pos = boost::next(prev);
            }
            next = boost::next(pos);
            BOOST_DEDUCED_TYPENAME Container::key_type
                key = test::get_key<Container>(*pos);
            std::size_t count = x.count(key);
            BOOST_TEST(next == x.erase(pos));
            --size;
            if(size > 0)
                BOOST_TEST(index == 0 ? next == x.begin() :
                        next == boost::next(prev));
            BOOST_TEST(x.count(key) == count - 1);
            BOOST_TEST(x.size() == size);
            if (++iterations % 20 == 0) test::check_equivalent_keys(x);
        }
        BOOST_TEST(x.empty());
    }

    std::cerr<<"erase(ranges).\n";
    {
        test::check_instances check_;

        test::random_values<Container> v(500, generator);
        Container x(v.begin(), v.end());

        std::size_t size = x.size();

        // I'm actually stretching it a little here, as the standard says it
        // returns 'the iterator immediately following the erase elements'
        // and if nothing is erased, then there's nothing to follow. But I
        // think this is the only sensible option...
        BOOST_TEST(x.erase(x.end(), x.end()) == x.end());
        BOOST_TEST(x.erase(x.begin(), x.begin()) == x.begin());
        BOOST_TEST(x.size() == size);
        test::check_equivalent_keys(x);

        BOOST_TEST(x.erase(x.begin(), x.end()) == x.end());
        BOOST_TEST(x.empty());
        BOOST_TEST(x.begin() == x.end());
        test::check_equivalent_keys(x);

        BOOST_TEST(x.erase(x.begin(), x.end()) == x.begin());
        test::check_equivalent_keys(x);
    }

    std::cerr<<"quick_erase(begin()).\n";
    {
        test::check_instances check_;

        test::random_values<Container> v(1000, generator);
        Container x(v.begin(), v.end());
        std::size_t size = x.size();
        int iterations = 0;
        while(size > 0 && !x.empty())
        {
            BOOST_DEDUCED_TYPENAME Container::key_type
                key = test::get_key<Container>(*x.begin());
            std::size_t count = x.count(key);
            x.quick_erase(x.begin());
            --size;
            BOOST_TEST(x.count(key) == count - 1);
            BOOST_TEST(x.size() == size);
            if (++iterations % 20 == 0) test::check_equivalent_keys(x);
        }
        BOOST_TEST(x.empty());
    }

    std::cerr<<"quick_erase(random position).\n";
    {
        test::check_instances check_;

        test::random_values<Container> v(1000, generator);
        Container x(v.begin(), v.end());
        std::size_t size = x.size();
        int iterations = 0;
        while(size > 0 && !x.empty())
        {
            using namespace std;
            int index = rand() % (int) x.size();
            BOOST_DEDUCED_TYPENAME Container::const_iterator prev, pos, next;
            if(index == 0) {
                prev = pos = x.begin();
            }
            else {
                prev = boost::next(x.begin(), index - 1);
                pos = boost::next(prev);
            }
            next = boost::next(pos);
            BOOST_DEDUCED_TYPENAME Container::key_type
                key = test::get_key<Container>(*pos);
            std::size_t count = x.count(key);
            x.quick_erase(pos);
            --size;
            if(size > 0)
                BOOST_TEST(index == 0 ? next == x.begin() :
                        next == boost::next(prev));
            BOOST_TEST(x.count(key) == count - 1);
            BOOST_TEST(x.size() == size);
            if (++iterations % 20 == 0) test::check_equivalent_keys(x);
        }
        BOOST_TEST(x.empty());
    }


    std::cerr<<"clear().\n";
    {
        test::check_instances check_;

        test::random_values<Container> v(500, generator);
        Container x(v.begin(), v.end());
        x.clear();
        BOOST_TEST(x.empty());
        BOOST_TEST(x.begin() == x.end());
    }

    std::cerr<<"\n";
}

boost::unordered_set<test::object,
    test::hash, test::equal_to,
    test::allocator1<test::object> >* test_set;
boost::unordered_multiset<test::object,
    test::hash, test::equal_to,
    test::allocator2<test::object> >* test_multiset;
boost::unordered_map<test::object, test::object,
    test::hash, test::equal_to,
    test::allocator1<test::object> >* test_map;
boost::unordered_multimap<test::object, test::object,
    test::hash, test::equal_to,
    test::allocator2<test::object> >* test_multimap;

using test::default_generator;
using test::generate_collisions;

UNORDERED_TEST(erase_tests1,
    ((test_set)(test_multiset)(test_map)(test_multimap))
    ((default_generator)(generate_collisions))
)

}

RUN_TESTS()
