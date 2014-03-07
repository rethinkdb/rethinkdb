
// Copyright 2006-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "../helpers/prefix.hpp"
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include "../helpers/postfix.hpp"

#include <boost/config.hpp>
#include <algorithm>
#include <iterator>
#include "../helpers/test.hpp"
#include "../objects/test.hpp"
#include "../objects/cxx11_allocator.hpp"
#include "../helpers/random_values.hpp"
#include "../helpers/tracker.hpp"
#include "../helpers/invariants.hpp"

#if defined(BOOST_MSVC)
#pragma warning(disable:4127) // conditional expression is constant
#endif

namespace swap_tests
{

test::seed_t initialize_seed(783472);

template <class X>
void swap_test_impl(X& x1, X& x2)
{
    test::ordered<X> tracker1 = test::create_ordered(x1);
    test::ordered<X> tracker2 = test::create_ordered(x2);
    tracker1.insert_range(x1.begin(), x1.end());
    tracker2.insert_range(x2.begin(), x2.end());
    x1.swap(x2);
    tracker1.compare(x2);
    tracker2.compare(x1);
}

template <class X>
void swap_tests1(X*, test::random_generator generator)
{
    {
        test::check_instances check_;

        X x;
        swap_test_impl(x, x);
    }

    {
        test::check_instances check_;

        X x,y;
        swap_test_impl(x, y);
    }

    {
        test::check_instances check_;

        test::random_values<X> v(1000, generator);
        X x, y(v.begin(), v.end());
        swap_test_impl(x, y);
        swap_test_impl(x, y);
    }

    {
        test::check_instances check_;

        test::random_values<X> vx(1000, generator), vy(1000, generator);
        X x(vx.begin(), vx.end()), y(vy.begin(), vy.end());
        swap_test_impl(x, y);
        swap_test_impl(x, y);
    }
}

template <class X>
void swap_tests2(X* ptr, test::random_generator generator)
{
    swap_tests1(ptr, generator);

    typedef BOOST_DEDUCED_TYPENAME X::hasher hasher;
    typedef BOOST_DEDUCED_TYPENAME X::key_equal key_equal;
    typedef BOOST_DEDUCED_TYPENAME X::allocator_type allocator_type;

    {
        test::check_instances check_;

        X x(0, hasher(1), key_equal(1));
        X y(0, hasher(2), key_equal(2));
        swap_test_impl(x, y);
    }

    {
        test::check_instances check_;

        test::random_values<X> v(1000, generator);
        X x(v.begin(), v.end(), 0, hasher(1), key_equal(1));
        X y(0, hasher(2), key_equal(2));
        swap_test_impl(x, y);
    }

    {
        test::check_instances check_;

        test::random_values<X> vx(100, generator), vy(50, generator);
        X x(vx.begin(), vx.end(), 0, hasher(1), key_equal(1));
        X y(vy.begin(), vy.end(), 0, hasher(2), key_equal(2));
        swap_test_impl(x, y);
        swap_test_impl(x, y);
    }

    {
        test::force_equal_allocator force_(
            !allocator_type::is_propagate_on_swap);
        test::check_instances check_;

        test::random_values<X> vx(50, generator), vy(100, generator);
        X x(vx.begin(), vx.end(), 0, hasher(), key_equal(), allocator_type(1));
        X y(vy.begin(), vy.end(), 0, hasher(), key_equal(), allocator_type(2));

        if (allocator_type::is_propagate_on_swap ||
            x.get_allocator() == y.get_allocator())
        {
            swap_test_impl(x, y);
        }
    }

    {
        test::force_equal_allocator force_(
            !allocator_type::is_propagate_on_swap);
        test::check_instances check_;

        test::random_values<X> vx(100, generator), vy(100, generator);
        X x(vx.begin(), vx.end(), 0, hasher(1), key_equal(1),
            allocator_type(1));
        X y(vy.begin(), vy.end(), 0, hasher(2), key_equal(2),
            allocator_type(2));

        if (allocator_type::is_propagate_on_swap ||
            x.get_allocator() == y.get_allocator())
        {
            swap_test_impl(x, y);
            swap_test_impl(x, y);
        }
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
        test::allocator1<test::object> >* test_map;
boost::unordered_multimap<test::object, test::object,
        test::hash, test::equal_to,
        test::allocator2<test::object> >* test_multimap;

boost::unordered_set<test::object,
        test::hash, test::equal_to,
        test::cxx11_allocator<test::object, test::propagate_swap> >*
    test_set_prop_swap;
boost::unordered_multiset<test::object,
        test::hash, test::equal_to,
        test::cxx11_allocator<test::object, test::propagate_swap> >*
    test_multiset_prop_swap;
boost::unordered_map<test::object, test::object,
        test::hash, test::equal_to,
        test::cxx11_allocator<test::object, test::propagate_swap> >*
    test_map_prop_swap;
boost::unordered_multimap<test::object, test::object,
        test::hash, test::equal_to,
        test::cxx11_allocator<test::object, test::propagate_swap> >*
    test_multimap_prop_swap;

boost::unordered_set<test::object,
        test::hash, test::equal_to,
        test::cxx11_allocator<test::object, test::no_propagate_swap> >*
    test_set_no_prop_swap;
boost::unordered_multiset<test::object,
        test::hash, test::equal_to,
        test::cxx11_allocator<test::object, test::no_propagate_swap> >*
    test_multiset_no_prop_swap;
boost::unordered_map<test::object, test::object,
        test::hash, test::equal_to,
        test::cxx11_allocator<test::object, test::no_propagate_swap> >*
    test_map_no_prop_swap;
boost::unordered_multimap<test::object, test::object,
        test::hash, test::equal_to,
        test::cxx11_allocator<test::object, test::no_propagate_swap> >*
    test_multimap_no_prop_swap;

template <typename T>
bool is_propagate(T*)
{
    return T::allocator_type::is_propagate_on_swap;
}

using test::default_generator;
using test::generate_collisions;

UNORDERED_AUTO_TEST(check_traits)
{
    BOOST_TEST(!is_propagate(test_set));
    BOOST_TEST(is_propagate(test_set_prop_swap));
    BOOST_TEST(!is_propagate(test_set_no_prop_swap));
}

UNORDERED_TEST(swap_tests1, (
        (test_map_std_alloc)
        (test_set)(test_multiset)(test_map)(test_multimap)
        (test_set_prop_swap)(test_multiset_prop_swap)(test_map_prop_swap)(test_multimap_prop_swap)
        (test_set_no_prop_swap)(test_multiset_no_prop_swap)(test_map_no_prop_swap)(test_multimap_no_prop_swap)
    )
    ((default_generator)(generate_collisions))
)

UNORDERED_TEST(swap_tests2, (
        (test_set)(test_multiset)(test_map)(test_multimap)
        (test_set_prop_swap)(test_multiset_prop_swap)(test_map_prop_swap)(test_multimap_prop_swap)
        (test_set_no_prop_swap)(test_multiset_no_prop_swap)(test_map_no_prop_swap)(test_multimap_no_prop_swap)
    )
    ((default_generator)(generate_collisions))
)

}
RUN_TESTS()
