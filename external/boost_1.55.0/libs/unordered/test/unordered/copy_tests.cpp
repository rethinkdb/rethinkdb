
// Copyright 2006-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "../helpers/prefix.hpp"
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include "../helpers/postfix.hpp"

#include "../helpers/test.hpp"
#include "../objects/test.hpp"
#include "../objects/cxx11_allocator.hpp"
#include "../helpers/random_values.hpp"
#include "../helpers/tracker.hpp"
#include "../helpers/equivalent.hpp"
#include "../helpers/invariants.hpp"

test::seed_t initialize_seed(9063);

namespace copy_tests
{

template <class T>
void copy_construct_tests1(T*, test::random_generator const& generator)
{
    typedef BOOST_DEDUCED_TYPENAME T::allocator_type allocator_type;

    BOOST_DEDUCED_TYPENAME T::hasher hf;
    BOOST_DEDUCED_TYPENAME T::key_equal eq;
    BOOST_DEDUCED_TYPENAME T::allocator_type al;    

    {
        test::check_instances check_;

        T x;
        T y(x);
        BOOST_TEST(y.empty());
        BOOST_TEST(test::equivalent(y.hash_function(), hf));
        BOOST_TEST(test::equivalent(y.key_eq(), eq));
        BOOST_TEST(test::equivalent(y.get_allocator(), al));
        BOOST_TEST(x.max_load_factor() == y.max_load_factor());
        BOOST_TEST(test::selected_count(y.get_allocator()) ==
            (allocator_type::is_select_on_copy));
        test::check_equivalent_keys(y);
    }

    {
        test::check_instances check_;

        test::random_values<T> v(1000, generator);

        T x(v.begin(), v.end());
        T y(x);
        test::unordered_equivalence_tester<T> equivalent(x);
        BOOST_TEST(equivalent(y));
        BOOST_TEST(test::selected_count(y.get_allocator()) ==
            (allocator_type::is_select_on_copy));
        test::check_equivalent_keys(y);
    }

    {
        test::check_instances check_;

        // In this test I drop the original containers max load factor, so it
        // is much lower than the load factor. The hash table is not allowed
        // to rehash, but the destination container should probably allocate
        // enough buckets to decrease the load factor appropriately.
        test::random_values<T> v(1000, generator);
        T x(v.begin(), v.end());
        x.max_load_factor(x.load_factor() / 4);
        T y(x);
        test::unordered_equivalence_tester<T> equivalent(x);
        BOOST_TEST(equivalent(y));
        // This isn't guaranteed:
        BOOST_TEST(y.load_factor() < y.max_load_factor());
        BOOST_TEST(test::selected_count(y.get_allocator()) ==
            (allocator_type::is_select_on_copy));
        test::check_equivalent_keys(y);
    }
}

template <class T>
void copy_construct_tests2(T*, test::random_generator const& generator)
{
    BOOST_DEDUCED_TYPENAME T::hasher hf(1);
    BOOST_DEDUCED_TYPENAME T::key_equal eq(1);
    BOOST_DEDUCED_TYPENAME T::allocator_type al(1);
    BOOST_DEDUCED_TYPENAME T::allocator_type al2(2);
    
    typedef BOOST_DEDUCED_TYPENAME T::allocator_type allocator_type;

    {
        test::check_instances check_;

        T x(10000, hf, eq, al);
        T y(x);
        BOOST_TEST(y.empty());
        BOOST_TEST(test::equivalent(y.hash_function(), hf));
        BOOST_TEST(test::equivalent(y.key_eq(), eq));
        BOOST_TEST(test::equivalent(y.get_allocator(), al));
        BOOST_TEST(x.max_load_factor() == y.max_load_factor());
        BOOST_TEST(test::selected_count(y.get_allocator()) ==
            (allocator_type::is_select_on_copy));
        test::check_equivalent_keys(y);
    }

    {
        test::check_instances check_;

        T x(1000, hf, eq, al);
        T y(x, al2);
        BOOST_TEST(y.empty());
        BOOST_TEST(test::equivalent(y.hash_function(), hf));
        BOOST_TEST(test::equivalent(y.key_eq(), eq));
        BOOST_TEST(test::equivalent(y.get_allocator(), al2));
        BOOST_TEST(x.max_load_factor() == y.max_load_factor());
        BOOST_TEST(test::selected_count(y.get_allocator()) == 0);
        test::check_equivalent_keys(y);
    }

    {
        test::check_instances check_;

        test::random_values<T> v(1000, generator);

        T x(v.begin(), v.end(), 0, hf, eq, al);
        T y(x);
        test::unordered_equivalence_tester<T> equivalent(x);
        BOOST_TEST(equivalent(y));
        test::check_equivalent_keys(y);
        BOOST_TEST(test::selected_count(y.get_allocator()) ==
            (allocator_type::is_select_on_copy));
        BOOST_TEST(test::equivalent(y.get_allocator(), al));
    }

    {
        test::check_instances check_;

        test::random_values<T> v(500, generator);

        T x(v.begin(), v.end(), 0, hf, eq, al);
        T y(x, al2);
        test::unordered_equivalence_tester<T> equivalent(x);
        BOOST_TEST(equivalent(y));
        test::check_equivalent_keys(y);
        BOOST_TEST(test::selected_count(y.get_allocator()) == 0);
        BOOST_TEST(test::equivalent(y.get_allocator(), al2));
    }
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

boost::unordered_set<test::object,
        test::hash, test::equal_to,
        test::cxx11_allocator<test::object, test::select_copy> >*
    test_set_select_copy;
boost::unordered_multiset<test::object,
        test::hash, test::equal_to,
        test::cxx11_allocator<test::object, test::select_copy> >*
    test_multiset_select_copy;
boost::unordered_map<test::object, test::object,
        test::hash, test::equal_to,
        test::cxx11_allocator<test::object, test::select_copy> >*
    test_map_select_copy;
boost::unordered_multimap<test::object, test::object,
        test::hash, test::equal_to,
        test::cxx11_allocator<test::object, test::select_copy> >*
    test_multimap_select_copy;

boost::unordered_set<test::object,
        test::hash, test::equal_to,
        test::cxx11_allocator<test::object, test::no_select_copy> >*
    test_set_no_select_copy;
boost::unordered_multiset<test::object,
        test::hash, test::equal_to,
        test::cxx11_allocator<test::object, test::no_select_copy> >*
    test_multiset_no_select_copy;
boost::unordered_map<test::object, test::object,
        test::hash, test::equal_to,
        test::cxx11_allocator<test::object, test::no_select_copy> >*
    test_map_no_select_copy;
boost::unordered_multimap<test::object, test::object,
        test::hash, test::equal_to,
        test::cxx11_allocator<test::object, test::no_select_copy> >*
    test_multimap_no_select_copy;

using test::default_generator;
using test::generate_collisions;

UNORDERED_TEST(copy_construct_tests1, (
        (test_set)(test_multiset)(test_map)(test_multimap)
        (test_set_select_copy)(test_multiset_select_copy)(test_map_select_copy)(test_multimap_select_copy)
        (test_set_no_select_copy)(test_multiset_no_select_copy)(test_map_no_select_copy)(test_multimap_no_select_copy)
    )
    ((default_generator)(generate_collisions))
)

UNORDERED_TEST(copy_construct_tests2, (
        (test_set)(test_multiset)(test_map)(test_multimap)
        (test_set_select_copy)(test_multiset_select_copy)(test_map_select_copy)(test_multimap_select_copy)
        (test_set_no_select_copy)(test_multiset_no_select_copy)(test_map_no_select_copy)(test_multimap_no_select_copy)
    )
    ((default_generator)(generate_collisions))
)

}

RUN_TESTS()
