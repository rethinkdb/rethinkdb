
// Copyright 2008-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or move at http://www.boost.org/LICENSE_1_0.txt)

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

#if defined(BOOST_MSVC)
#pragma warning(disable:4127) // conditional expression is constant
#endif

namespace move_tests
{
    test::seed_t initialize_seed(98624);
#if defined(BOOST_UNORDERED_USE_MOVE) || !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
#define BOOST_UNORDERED_TEST_MOVING 1
#else
#define BOOST_UNORDERED_TEST_MOVING 0
#endif

    template<class T>
    T empty(T*) {
        return T();
    }

    template<class T>
    T create(test::random_values<T> const& v,
            test::object_count& count) {
        T x(v.begin(), v.end());
        count = test::global_object_count;
        return x;
    }

    template<class T>
    T create(test::random_values<T> const& v,
            test::object_count& count,
            BOOST_DEDUCED_TYPENAME T::hasher hf,
            BOOST_DEDUCED_TYPENAME T::key_equal eq,
            BOOST_DEDUCED_TYPENAME T::allocator_type al,
            float mlf) {
        T x(0, hf, eq, al);
        x.max_load_factor(mlf);
        x.insert(v.begin(), v.end());
        count = test::global_object_count;
        return x;
    }

    template <class T>
    void move_construct_tests1(T* ptr, test::random_generator const& generator)
    {
        BOOST_DEDUCED_TYPENAME T::hasher hf;
        BOOST_DEDUCED_TYPENAME T::key_equal eq;
        BOOST_DEDUCED_TYPENAME T::allocator_type al;

        {
            test::check_instances check_;

            T y(empty(ptr));
            BOOST_TEST(y.empty());
            BOOST_TEST(test::equivalent(y.hash_function(), hf));
            BOOST_TEST(test::equivalent(y.key_eq(), eq));
            BOOST_TEST(test::equivalent(y.get_allocator(), al));
            BOOST_TEST(y.max_load_factor() == 1.0);
            test::check_equivalent_keys(y);
        }

        {
            test::check_instances check_;

            test::random_values<T> v(1000, generator);
            test::object_count count;
            T y(create(v, count));
#if defined(BOOST_HAS_NRVO)
            BOOST_TEST(count == test::global_object_count);
#endif
            test::check_container(y, v);
            test::check_equivalent_keys(y);
        }
    }

    template <class T>
    void move_assign_tests1(T*, test::random_generator const& generator)
    {
        {
            test::check_instances check_;

            test::random_values<T> v(500, generator);
            test::object_count count;
            T y;
            y = create(v, count);
#if BOOST_UNORDERED_TEST_MOVING && defined(BOOST_HAS_NRVO)
            BOOST_TEST(count == test::global_object_count);
#endif
            test::check_container(y, v);
            test::check_equivalent_keys(y);
        }
    }

    template <class T>
    void move_construct_tests2(T*, test::random_generator const& generator)
    {
        BOOST_DEDUCED_TYPENAME T::hasher hf(1);
        BOOST_DEDUCED_TYPENAME T::key_equal eq(1);
        BOOST_DEDUCED_TYPENAME T::allocator_type al(1);
        BOOST_DEDUCED_TYPENAME T::allocator_type al2(2);

        test::object_count count;

        {
            test::check_instances check_;

            test::random_values<T> v(500, generator);
            T y(create(v, count, hf, eq, al, 0.5));
#if defined(BOOST_HAS_NRVO)
            BOOST_TEST(count == test::global_object_count);
#endif
            test::check_container(y, v);
            BOOST_TEST(test::equivalent(y.hash_function(), hf));
            BOOST_TEST(test::equivalent(y.key_eq(), eq));
            BOOST_TEST(test::equivalent(y.get_allocator(), al));
            BOOST_TEST(y.max_load_factor() == 0.5); // Not necessarily required.
            test::check_equivalent_keys(y);
        }

        {
            test::check_instances check_;

            // TODO: To do this correctly requires the fancy new allocator
            // stuff.
            test::random_values<T> v(500, generator);
            T y(create(v, count, hf, eq, al, 2.0), al2);
            BOOST_TEST(count != test::global_object_count);
            test::check_container(y, v);
            BOOST_TEST(test::equivalent(y.hash_function(), hf));
            BOOST_TEST(test::equivalent(y.key_eq(), eq));
            BOOST_TEST(test::equivalent(y.get_allocator(), al2));
            BOOST_TEST(y.max_load_factor() == 2.0); // Not necessarily required.
            test::check_equivalent_keys(y);
        }

        {
            test::check_instances check_;

            test::random_values<T> v(25, generator);
            T y(create(v, count, hf, eq, al, 1.0), al);
#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
            BOOST_TEST(count == test::global_object_count);
#elif defined(BOOST_HAS_NRVO)
            BOOST_TEST(
                static_cast<std::size_t>(test::global_object_count.constructions
                    - count.constructions) <=
                (test::is_set<T>::value ? 1 : 2) *
                    (test::has_unique_keys<T>::value ? 25 : v.size()));
            BOOST_TEST(count.instances == test::global_object_count.instances);
#else
            BOOST_TEST(
                static_cast<std::size_t>(test::global_object_count.constructions
                    - count.constructions) <=
                (test::is_set<T>::value ? 2 : 4) *
                    (test::has_unique_keys<T>::value ? 25 : v.size()));
            BOOST_TEST(count.instances == test::global_object_count.instances);
#endif
            test::check_container(y, v);
            BOOST_TEST(test::equivalent(y.hash_function(), hf));
            BOOST_TEST(test::equivalent(y.key_eq(), eq));
            BOOST_TEST(test::equivalent(y.get_allocator(), al));
            BOOST_TEST(y.max_load_factor() == 1.0); // Not necessarily required.
            test::check_equivalent_keys(y);
        }
    }

    template <class T>
    void move_assign_tests2(T*, test::random_generator const& generator)
    {
        BOOST_DEDUCED_TYPENAME T::hasher hf(1);
        BOOST_DEDUCED_TYPENAME T::key_equal eq(1);
        BOOST_DEDUCED_TYPENAME T::allocator_type al1(1);
        BOOST_DEDUCED_TYPENAME T::allocator_type al2(2);
        typedef BOOST_DEDUCED_TYPENAME T::allocator_type allocator_type;

        {
            test::random_values<T> v(500, generator);
            test::random_values<T> v2(0, generator);
            T y(v.begin(), v.end(), 0, hf, eq, al1);
            test::object_count count;
            y = create(v2, count, hf, eq, al2, 2.0);
            BOOST_TEST(y.empty());
            test::check_container(y, v2);
            test::check_equivalent_keys(y);
            BOOST_TEST(y.max_load_factor() == 2.0);

#if defined(BOOST_HAS_NRVO)
            if (BOOST_UNORDERED_TEST_MOVING ?
                    (bool) allocator_type::is_propagate_on_move :
                    (bool) allocator_type::is_propagate_on_assign)
            {
                BOOST_TEST(test::equivalent(y.get_allocator(), al2));
            }
            else {
                BOOST_TEST(test::equivalent(y.get_allocator(), al1));
            }
#endif
        }

        {
            test::random_values<T> v(500, generator);
            test::object_count count;
            T y(0, hf, eq, al1);
            y = create(v, count, hf, eq, al2, 0.5);
#if defined(BOOST_HAS_NRVO)
            if (BOOST_UNORDERED_TEST_MOVING &&
                    allocator_type::is_propagate_on_move)
            {
                BOOST_TEST(count == test::global_object_count);
            }
#endif
            test::check_container(y, v);
            test::check_equivalent_keys(y);
            BOOST_TEST(y.max_load_factor() == 0.5);

#if defined(BOOST_HAS_NRVO)
            if (BOOST_UNORDERED_TEST_MOVING ?
                    (bool) allocator_type::is_propagate_on_move :
                    (bool) allocator_type::is_propagate_on_assign)
            {
                BOOST_TEST(test::equivalent(y.get_allocator(), al2));
            }
            else {
                BOOST_TEST(test::equivalent(y.get_allocator(), al1));
            }
#endif
        }

        {
            test::check_instances check_;

            test::random_values<T> v(500, generator);
            T y(0, hf, eq, al1);

            T x(0, hf, eq, al2);
            x.max_load_factor(0.25);
            x.insert(v.begin(), v.end());

            test::object_count count = test::global_object_count;
            y = boost::move(x);
            if (BOOST_UNORDERED_TEST_MOVING &&
                    allocator_type::is_propagate_on_move)
            {
                BOOST_TEST(count == test::global_object_count);
            }
            test::check_container(y, v);
            test::check_equivalent_keys(y);
            BOOST_TEST(y.max_load_factor() == 0.25);

            if (BOOST_UNORDERED_TEST_MOVING ?
                    (bool) allocator_type::is_propagate_on_move :
                    (bool) allocator_type::is_propagate_on_assign)
            {
                BOOST_TEST(test::equivalent(y.get_allocator(), al2));
            }
            else {
                BOOST_TEST(test::equivalent(y.get_allocator(), al1));
            }
        }

        {
            test::check_instances check_;

            test::random_values<T> v1(1000, generator);
            test::random_values<T> v2(200, generator);

            T x(0, hf, eq, al2);
            x.max_load_factor(0.5);
            x.insert(v2.begin(), v2.end());

            test::object_count count1 = test::global_object_count;

            T y(v1.begin(), v1.end(), 0, hf, eq, al1);
            y = boost::move(x);

            test::object_count count2 = test::global_object_count;

            if (BOOST_UNORDERED_TEST_MOVING &&
                    allocator_type::is_propagate_on_move)
            {
                BOOST_TEST(count1.instances ==
                    test::global_object_count.instances);
                BOOST_TEST(count2.constructions ==
                    test::global_object_count.constructions);
            }

            test::check_container(y, v2);
            test::check_equivalent_keys(y);
            BOOST_TEST(y.max_load_factor() == 0.5);

            if (BOOST_UNORDERED_TEST_MOVING ?
                    (bool) allocator_type::is_propagate_on_move :
                    (bool) allocator_type::is_propagate_on_assign)
            {
                BOOST_TEST(test::equivalent(y.get_allocator(), al2));
            }
            else {
                BOOST_TEST(test::equivalent(y.get_allocator(), al1));
            }
        }
    }

    boost::unordered_map<test::object, test::object,
        test::hash, test::equal_to,
        std::allocator<test::object> >* test_map_std_alloc;

    boost::unordered_set<test::object,
        test::hash, test::equal_to,
        test::allocator2<test::object> >* test_set;
    boost::unordered_multiset<test::object,
        test::hash, test::equal_to,
        test::allocator1<test::object> >* test_multiset;
    boost::unordered_map<test::object, test::object,
        test::hash, test::equal_to,
        test::allocator1<test::object> >* test_map;
    boost::unordered_multimap<test::object, test::object,
        test::hash, test::equal_to,
        test::allocator2<test::object> >* test_multimap;

boost::unordered_set<test::object,
        test::hash, test::equal_to,
        test::cxx11_allocator<test::object, test::propagate_move> >*
    test_set_prop_move;
boost::unordered_multiset<test::object,
        test::hash, test::equal_to,
        test::cxx11_allocator<test::object, test::propagate_move> >*
    test_multiset_prop_move;
boost::unordered_map<test::object, test::object,
        test::hash, test::equal_to,
        test::cxx11_allocator<test::object, test::propagate_move> >*
    test_map_prop_move;
boost::unordered_multimap<test::object, test::object,
        test::hash, test::equal_to,
        test::cxx11_allocator<test::object, test::propagate_move> >*
    test_multimap_prop_move;

boost::unordered_set<test::object,
        test::hash, test::equal_to,
        test::cxx11_allocator<test::object, test::no_propagate_move> >*
    test_set_no_prop_move;
boost::unordered_multiset<test::object,
        test::hash, test::equal_to,
        test::cxx11_allocator<test::object, test::no_propagate_move> >*
    test_multiset_no_prop_move;
boost::unordered_map<test::object, test::object,
        test::hash, test::equal_to,
        test::cxx11_allocator<test::object, test::no_propagate_move> >*
    test_map_no_prop_move;
boost::unordered_multimap<test::object, test::object,
        test::hash, test::equal_to,
        test::cxx11_allocator<test::object, test::no_propagate_move> >*
    test_multimap_no_prop_move;

    using test::default_generator;
    using test::generate_collisions;

    UNORDERED_TEST(move_construct_tests1, (
            (test_map_std_alloc)
            (test_set)(test_multiset)(test_map)(test_multimap)
            (test_set_prop_move)(test_multiset_prop_move)(test_map_prop_move)(test_multimap_prop_move)
            (test_set_no_prop_move)(test_multiset_no_prop_move)(test_map_no_prop_move)(test_multimap_no_prop_move)
        )
        ((default_generator)(generate_collisions))
    )
    UNORDERED_TEST(move_assign_tests1, (
            (test_map_std_alloc)
            (test_set)(test_multiset)(test_map)(test_multimap)
            (test_set_prop_move)(test_multiset_prop_move)(test_map_prop_move)(test_multimap_prop_move)
            (test_set_no_prop_move)(test_multiset_no_prop_move)(test_map_no_prop_move)(test_multimap_no_prop_move)
        )
        ((default_generator)(generate_collisions))
    )
    UNORDERED_TEST(move_construct_tests2, (
            (test_set)(test_multiset)(test_map)(test_multimap)
            (test_set_prop_move)(test_multiset_prop_move)(test_map_prop_move)(test_multimap_prop_move)
            (test_set_no_prop_move)(test_multiset_no_prop_move)(test_map_no_prop_move)(test_multimap_no_prop_move)
        )
        ((default_generator)(generate_collisions))
    )
    UNORDERED_TEST(move_assign_tests2, (
            (test_set)(test_multiset)(test_map)(test_multimap)
            (test_set_prop_move)(test_multiset_prop_move)(test_map_prop_move)(test_multimap_prop_move)
            (test_set_no_prop_move)(test_multiset_no_prop_move)(test_map_no_prop_move)(test_multimap_no_prop_move)
        )
        ((default_generator)(generate_collisions))
    )
}

RUN_TESTS()
