
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

#include <iostream>

#if defined(BOOST_MSVC)
#pragma warning(disable:4127) // conditional expression is constant
#endif

namespace assign_tests {

test::seed_t initialize_seed(96785);

template <class T>
void assign_tests1(T*, test::random_generator generator)
{
    BOOST_DEDUCED_TYPENAME T::hasher hf;
    BOOST_DEDUCED_TYPENAME T::key_equal eq;

    std::cerr<<"assign_tests1.1\n";
    {
        test::check_instances check_;

        T x;
        x = x;
        BOOST_TEST(x.empty());
        BOOST_TEST(test::equivalent(x.hash_function(), hf));
        BOOST_TEST(test::equivalent(x.key_eq(), eq));
    }

    std::cerr<<"assign_tests1.2\n";
    {
        test::check_instances check_;

        test::random_values<T> v(1000, generator);
        T x(v.begin(), v.end());

        test::ordered<T> tracker = test::create_ordered(x);
        tracker.insert_range(v.begin(), v.end());

        x = x;
        tracker.compare(x);

        T y;
        y.max_load_factor(x.max_load_factor() / 20);
        float mlf = x.max_load_factor();
        y = x;
        tracker.compare(x);
        tracker.compare(y);
        BOOST_TEST(x.max_load_factor() == mlf);
        BOOST_TEST(y.max_load_factor() == mlf);
        BOOST_TEST(y.load_factor() <= y.max_load_factor());
    }
}

template <class T>
void assign_tests2(T*, test::random_generator generator)
{
    BOOST_DEDUCED_TYPENAME T::hasher hf1(1);
    BOOST_DEDUCED_TYPENAME T::hasher hf2(2);
    BOOST_DEDUCED_TYPENAME T::key_equal eq1(1);
    BOOST_DEDUCED_TYPENAME T::key_equal eq2(2);
    BOOST_DEDUCED_TYPENAME T::allocator_type al1(1);
    BOOST_DEDUCED_TYPENAME T::allocator_type al2(2);
    
    typedef BOOST_DEDUCED_TYPENAME T::allocator_type allocator_type;
    
    std::cerr<<"assign_tests2.1\n";
    {
        test::check_instances check_;

        test::random_values<T> v(1000, generator);
        T x1(v.begin(), v.end(), 0, hf1, eq1);
        T x2(0, hf2, eq2);
        x2 = x1;
        BOOST_TEST(test::equivalent(x1.hash_function(), hf1));
        BOOST_TEST(test::equivalent(x1.key_eq(), eq1));
        BOOST_TEST(test::equivalent(x2.hash_function(), hf1));
        BOOST_TEST(test::equivalent(x2.key_eq(), eq1));
        test::check_container(x1, v);
        test::check_container(x2, v);
        BOOST_TEST(x2.load_factor() <= x2.max_load_factor());
    }

    std::cerr<<"assign_tests2.1a\n";
    {
        test::check_instances check_;

        test::random_values<T> v1(0, generator);
        test::random_values<T> v2(1000, generator);
        T x1(0, hf2, eq2);
        T x2(v2.begin(), v2.end(), 0, hf1, eq1);
        x2 = x1;
        BOOST_TEST(test::equivalent(x1.hash_function(), hf2));
        BOOST_TEST(test::equivalent(x1.key_eq(), eq2));
        BOOST_TEST(test::equivalent(x2.hash_function(), hf2));
        BOOST_TEST(test::equivalent(x2.key_eq(), eq2));
        test::check_container(x1, v1);
        test::check_container(x2, v1);
        BOOST_TEST(x2.load_factor() <= x2.max_load_factor());
    }

    std::cerr<<"assign_tests2.2\n";
    {
        test::check_instances check_;

        test::random_values<T> v1(100, generator), v2(100, generator);
        T x1(v1.begin(), v1.end(), 0, hf1, eq1, al1);
        T x2(v2.begin(), v2.end(), 0, hf2, eq2, al2);
        x2 = x1;
        BOOST_TEST(test::equivalent(x2.hash_function(), hf1));
        BOOST_TEST(test::equivalent(x2.key_eq(), eq1));
        if (allocator_type::is_propagate_on_assign) {
            BOOST_TEST(test::equivalent(x2.get_allocator(), al1));
            BOOST_TEST(!test::equivalent(x2.get_allocator(), al2));
        }
        else {
            BOOST_TEST(test::equivalent(x2.get_allocator(), al2));
            BOOST_TEST(!test::equivalent(x2.get_allocator(), al1));
        }
        test::check_container(x1, v1);
        test::check_container(x2, v1);
        BOOST_TEST(x2.load_factor() <= x2.max_load_factor());
    }

    std::cerr<<"assign_tests2.3\n";
    {
        test::check_instances check_;

        test::random_values<T> v1(100, generator), v2(1000, generator);
        T x1(v1.begin(), v1.end(), 0, hf1, eq1, al1);
        T x2(v2.begin(), v2.end(), 0, hf2, eq2, al2);
        x2 = x1;
        BOOST_TEST(test::equivalent(x2.hash_function(), hf1));
        BOOST_TEST(test::equivalent(x2.key_eq(), eq1));
        if (allocator_type::is_propagate_on_assign) {
            BOOST_TEST(test::equivalent(x2.get_allocator(), al1));
            BOOST_TEST(!test::equivalent(x2.get_allocator(), al2));
        }
        else {
            BOOST_TEST(test::equivalent(x2.get_allocator(), al2));
            BOOST_TEST(!test::equivalent(x2.get_allocator(), al1));
        }
        test::check_container(x1, v1);
        test::check_container(x2, v1);
        BOOST_TEST(x2.load_factor() <= x2.max_load_factor());
    }

    std::cerr<<"assign_tests2.4\n";
    {
        test::check_instances check_;

        test::random_values<T> v1(1000, generator), v2(100, generator);
        T x1(v1.begin(), v1.end(), 0, hf1, eq1, al1);
        T x2(v2.begin(), v2.end(), 0, hf2, eq2, al2);
        x2 = x1;
        BOOST_TEST(test::equivalent(x2.hash_function(), hf1));
        BOOST_TEST(test::equivalent(x2.key_eq(), eq1));
        if (allocator_type::is_propagate_on_assign) {
            BOOST_TEST(test::equivalent(x2.get_allocator(), al1));
            BOOST_TEST(!test::equivalent(x2.get_allocator(), al2));
        }
        else {
            BOOST_TEST(test::equivalent(x2.get_allocator(), al2));
            BOOST_TEST(!test::equivalent(x2.get_allocator(), al1));
        }
        test::check_container(x1, v1);
        test::check_container(x2, v1);
        BOOST_TEST(x2.load_factor() <= x2.max_load_factor());
    }
}

boost::unordered_map<test::object, test::object,
    test::hash, test::equal_to,
    std::allocator<test::object> >* test_map_std_alloc;

boost::unordered_set<test::object,
    test::hash, test::equal_to,
    test::allocator1<test::object> >* test_set;
boost::unordered_multiset<test::object,
    test::hash, test::equal_to,
    test::allocator2<test::object> >* test_multiset;
boost::unordered_map<test::object, test::object,
    test::hash, test::equal_to,
    test::allocator2<test::object> >* test_map;
boost::unordered_multimap<test::object, test::object,
    test::hash, test::equal_to,
    test::allocator1<test::object> >* test_multimap;

boost::unordered_set<test::object,
        test::hash, test::equal_to,
        test::cxx11_allocator<test::object, test::propagate_assign> >*
    test_set_prop_assign;
boost::unordered_multiset<test::object,
        test::hash, test::equal_to,
        test::cxx11_allocator<test::object, test::propagate_assign> >*
    test_multiset_prop_assign;
boost::unordered_map<test::object, test::object,
        test::hash, test::equal_to,
        test::cxx11_allocator<test::object, test::propagate_assign> >*
    test_map_prop_assign;
boost::unordered_multimap<test::object, test::object,
        test::hash, test::equal_to,
        test::cxx11_allocator<test::object, test::propagate_assign> >*
    test_multimap_prop_assign;

boost::unordered_set<test::object,
        test::hash, test::equal_to,
        test::cxx11_allocator<test::object, test::no_propagate_assign> >*
    test_set_no_prop_assign;
boost::unordered_multiset<test::object,
        test::hash, test::equal_to,
        test::cxx11_allocator<test::object, test::no_propagate_assign> >*
    test_multiset_no_prop_assign;
boost::unordered_map<test::object, test::object,
        test::hash, test::equal_to,
        test::cxx11_allocator<test::object, test::no_propagate_assign> >*
    test_map_no_prop_assign;
boost::unordered_multimap<test::object, test::object,
        test::hash, test::equal_to,
        test::cxx11_allocator<test::object, test::no_propagate_assign> >*
    test_multimap_no_prop_assign;

using test::default_generator;
using test::generate_collisions;

template <typename T>
bool is_propagate(T*)
{
    return T::allocator_type::is_propagate_on_assign;
}

UNORDERED_AUTO_TEST(check_traits)
{
    BOOST_TEST(!is_propagate(test_set));
    BOOST_TEST(is_propagate(test_set_prop_assign));
    BOOST_TEST(!is_propagate(test_set_no_prop_assign));
}

UNORDERED_TEST(assign_tests1, (
        (test_map_std_alloc)
        (test_set)(test_multiset)(test_map)(test_multimap)
        (test_set_prop_assign)(test_multiset_prop_assign)(test_map_prop_assign)(test_multimap_prop_assign)
        (test_set_no_prop_assign)(test_multiset_no_prop_assign)(test_map_no_prop_assign)(test_multimap_no_prop_assign)
    )
    ((default_generator)(generate_collisions))
)

UNORDERED_TEST(assign_tests2, (
        (test_set)(test_multiset)(test_map)(test_multimap)
        (test_set_prop_assign)(test_multiset_prop_assign)(test_map_prop_assign)(test_multimap_prop_assign)
        (test_set_no_prop_assign)(test_multiset_no_prop_assign)(test_map_no_prop_assign)(test_multimap_no_prop_assign)
    )
    ((default_generator)(generate_collisions))
)

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)

UNORDERED_AUTO_TEST(assign_default_initializer_list) {
    std::cerr<<"Initializer List Tests\n";
    std::initializer_list<std::pair<int const, int> > init;
    boost::unordered_map<int, int> x1;
    x1[25] = 3;
    x1[16] = 10;
    BOOST_TEST(!x1.empty());
    x1 = init;
    BOOST_TEST(x1.empty());
}

#endif

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
UNORDERED_AUTO_TEST(assign_initializer_list)
{
    std::cerr<<"Initializer List Tests\n";

    boost::unordered_set<int> x;
    x.insert(10);
    x.insert(20);
    x = { 1, 2, -10 };
    BOOST_TEST(x.find(10) == x.end());
    BOOST_TEST(x.find(-10) != x.end());
}

#endif

}

RUN_TESTS()
