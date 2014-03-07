
// Copyright 2006-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "../helpers/prefix.hpp"
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include "../helpers/postfix.hpp"

#include "../helpers/test.hpp"
#include "../helpers/random_values.hpp"
#include "../helpers/tracker.hpp"
#include "../helpers/metafunctions.hpp"
#include "../objects/test.hpp"

namespace rehash_tests
{

test::seed_t initialize_seed(2974);

template <class X>
bool postcondition(X const& x, BOOST_DEDUCED_TYPENAME X::size_type n)
{
    return static_cast<double>(x.bucket_count()) >
        static_cast<double>(x.size()) / x.max_load_factor() &&
        x.bucket_count() >= n;
}

template <class X>
void rehash_empty_test1(X*)
{
    X x;

    x.rehash(10000);
    BOOST_TEST(postcondition(x, 10000));

    x.rehash(0);
    BOOST_TEST(postcondition(x, 0));

    x.rehash(10000000);
    BOOST_TEST(postcondition(x, 10000000));
}

template <class X>
void rehash_empty_test2(X*, test::random_generator generator)
{
    test::random_values<X> v(1000, generator);
    test::ordered<X> tracker;

    X x;

    x.rehash(10000);
    BOOST_TEST(postcondition(x, 10000));

    tracker.insert_range(v.begin(), v.end());
    x.insert(v.begin(), v.end());
    tracker.compare(x);

    BOOST_TEST(postcondition(x, 10000));
    
    x.rehash(10000000);
    tracker.compare(x);
    BOOST_TEST(postcondition(x, 10000000));
}

template <class X>
void rehash_empty_test3(X*, test::random_generator generator)
{
    test::random_values<X> v(1000, generator);
    test::ordered<X> tracker;

    X x;

    x.rehash(0);
    BOOST_TEST(postcondition(x, 0));

    tracker.insert_range(v.begin(), v.end());
    x.insert(v.begin(), v.end());
    tracker.compare(x);

    BOOST_TEST(postcondition(x, 0));
}

template <class X>
void rehash_test1(X*, test::random_generator generator)
{
    test::random_values<X> v(1000, generator);
    test::ordered<X> tracker;
    tracker.insert_range(v.begin(), v.end());
    X x(v.begin(), v.end());

    x.rehash(0); BOOST_TEST(postcondition(x, 0));
    tracker.compare(x);

    x.max_load_factor(0.25);
    x.rehash(0); BOOST_TEST(postcondition(x, 0));
    tracker.compare(x);

    x.max_load_factor(50.0);
    x.rehash(0); BOOST_TEST(postcondition(x, 0));
    tracker.compare(x);

    x.rehash(1000); BOOST_TEST(postcondition(x, 1000));
    tracker.compare(x);
}

template <class X>
void reserve_empty_test1(X*)
{
    X x;

    x.reserve(10000);
    BOOST_TEST(x.bucket_count() >= 10000);

    x.reserve(0);

    x.reserve(10000000);
    BOOST_TEST(x.bucket_count() >= 10000000);
}

template <class X>
void reserve_empty_test2(X*)
{
    X x;
    x.max_load_factor(0.25);

    x.reserve(10000);
    BOOST_TEST(x.bucket_count() >= 40000);

    x.reserve(0);

    x.reserve(10000000);
    BOOST_TEST(x.bucket_count() >= 40000000);
}

template <class X>
void reserve_test1(X*, test::random_generator generator)
{
    for (int random_mlf = 0; random_mlf < 2; ++random_mlf)
    {
        for (int i = 1; i < 2000; i += i < 50 ? 1 : 13)
        {
            test::random_values<X> v(i, generator);

            test::ordered<X> tracker;
            tracker.insert_range(v.begin(), v.end());

            X x;
            x.max_load_factor(random_mlf ?
                static_cast<float>(std::rand() % 1000) / 500.0f + 0.5f : 1.0f);
            // For the current standard this should reserve i+1, I've
            // submitted a defect report and will assume it's a defect
            // for now.
            x.reserve(test::has_unique_keys<X>::value ? i : v.size());

            // Insert an element before the range insert, otherwise there are
            // no iterators to invalidate in the range insert, and it can
            // rehash.
            typename test::random_values<X>::iterator it = v.begin();
            x.insert(*it);
            ++it;

            std::size_t bucket_count = x.bucket_count();
            x.insert(it, v.end());
            BOOST_TEST(bucket_count == x.bucket_count());
            tracker.compare(x);
        }
    }
}

template <class X>
void reserve_test2(X*, test::random_generator generator)
{
    for (int random_mlf = 0; random_mlf < 2; ++random_mlf)
    {
        for (int i = 0; i < 2000; i += i < 50 ? 1 : 13)
        {
            test::random_values<X> v(i, generator);

            test::ordered<X> tracker;
            tracker.insert_range(v.begin(), v.end());

            X x;
            x.max_load_factor(random_mlf ?
                static_cast<float>(std::rand() % 1000) / 500.0f + 0.5f : 1.0f);

            x.reserve(test::has_unique_keys<X>::value ? i : v.size());

            std::size_t bucket_count = x.bucket_count();
            for (typename test::random_values<X>::iterator it = v.begin();
                    it != v.end(); ++it)
            {
                x.insert(*it);
            }

            BOOST_TEST(bucket_count == x.bucket_count());
            tracker.compare(x);
        }
    }
}

boost::unordered_set<int>* int_set_ptr;
boost::unordered_multiset<test::object,
    test::hash, test::equal_to,
    test::allocator2<test::object> >* test_multiset_ptr;
boost::unordered_map<test::movable, test::movable,
    test::hash, test::equal_to,
    test::allocator2<test::movable> >* test_map_ptr;
boost::unordered_multimap<int, int>* int_multimap_ptr;

using test::default_generator;
using test::generate_collisions;

UNORDERED_TEST(rehash_empty_test1,
    ((int_set_ptr)(test_multiset_ptr)(test_map_ptr)(int_multimap_ptr))
)
UNORDERED_TEST(rehash_empty_test2,
    ((int_set_ptr)(test_multiset_ptr)(test_map_ptr)(int_multimap_ptr))
    ((default_generator)(generate_collisions))
)
UNORDERED_TEST(rehash_empty_test3,
    ((int_set_ptr)(test_multiset_ptr)(test_map_ptr)(int_multimap_ptr))
    ((default_generator)(generate_collisions))
)
UNORDERED_TEST(rehash_test1,
    ((int_set_ptr)(test_multiset_ptr)(test_map_ptr)(int_multimap_ptr))
    ((default_generator)(generate_collisions))
)
UNORDERED_TEST(reserve_empty_test1,
    ((int_set_ptr)(test_multiset_ptr)(test_map_ptr)(int_multimap_ptr))
)
UNORDERED_TEST(reserve_empty_test2,
    ((int_set_ptr)(test_multiset_ptr)(test_map_ptr)(int_multimap_ptr))
)
UNORDERED_TEST(reserve_test1,
    ((int_set_ptr)(test_multiset_ptr)(test_map_ptr)(int_multimap_ptr))
    ((default_generator)(generate_collisions))
)
UNORDERED_TEST(reserve_test2,
    ((int_set_ptr)(test_multiset_ptr)(test_map_ptr)(int_multimap_ptr))
    ((default_generator)(generate_collisions))
)

}

RUN_TESTS()
