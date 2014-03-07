
// Copyright 2006-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "../helpers/prefix.hpp"
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include "../helpers/postfix.hpp"

#include "../helpers/test.hpp"
#include <algorithm>
#include "../objects/test.hpp"
#include "../helpers/random_values.hpp"
#include "../helpers/helpers.hpp"

#if BOOST_WORKAROUND(BOOST_MSVC, < 1400)
#pragma warning(disable:4267) // conversion from 'size_t' to 'unsigned int',
                              // possible loss of data.
#endif

namespace bucket_tests {

test::seed_t initialize_seed(54635);

template <class X>
void tests(X*, test::random_generator generator)
{
    test::check_instances check_;

    typedef BOOST_DEDUCED_TYPENAME X::size_type size_type;
    typedef BOOST_DEDUCED_TYPENAME X::const_local_iterator const_local_iterator;
    test::random_values<X> v(1000, generator);

    X x(v.begin(), v.end());

    BOOST_TEST(x.bucket_count() < x.max_bucket_count());
    std::cerr<<x.bucket_count()<<"<"<<x.max_bucket_count()<<"\n";

    for(BOOST_DEDUCED_TYPENAME test::random_values<X>::const_iterator
            it = v.begin(), end = v.end(); it != end; ++it)
    {
        size_type bucket = x.bucket(test::get_key<X>(*it));

        BOOST_TEST(bucket < x.bucket_count());
        if(bucket < x.max_bucket_count()) {
            // lit? lend?? I need a new naming scheme.
            const_local_iterator lit = x.begin(bucket), lend = x.end(bucket);
            while(lit != lend
                && test::get_key<X>(*it) != test::get_key<X>(*lit))
            {
                ++lit;
            }
            BOOST_TEST(lit != lend);
        }
    }

    for(size_type i = 0; i < x.bucket_count(); ++i) {
        BOOST_TEST(x.bucket_size(i) == static_cast<size_type>(
                std::distance(x.begin(i), x.end(i))));
        BOOST_TEST(x.bucket_size(i) == static_cast<size_type>(
                std::distance(x.cbegin(i), x.cend(i))));
        X const& x_ref = x;
        BOOST_TEST(x.bucket_size(i) == static_cast<size_type>(
                std::distance(x_ref.begin(i), x_ref.end(i))));
        BOOST_TEST(x.bucket_size(i) == static_cast<size_type>(
                std::distance(x_ref.cbegin(i), x_ref.cend(i))));
    }
}

boost::unordered_multimap<test::object, test::object,
    test::hash, test::equal_to,
    std::allocator<test::object> >* test_multimap_std_alloc;

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

using test::default_generator;
using test::generate_collisions;

UNORDERED_TEST(tests,
    ((test_multimap_std_alloc)(test_set)(test_multiset)(test_map)(test_multimap))
    ((default_generator)(generate_collisions))
)

}

RUN_TESTS()
