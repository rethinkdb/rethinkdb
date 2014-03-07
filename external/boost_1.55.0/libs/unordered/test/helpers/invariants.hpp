
// Copyright 2006-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This header contains metafunctions/functions to get the equivalent
// associative container for an unordered container, and compare the contents.

#if !defined(BOOST_UNORDERED_TEST_HELPERS_INVARIANT_HEADER)
#define BOOST_UNORDERED_TEST_HELPERS_INVARIANT_HEADER

#include <set>
#include <cmath>
#include "./metafunctions.hpp"
#include "./helpers.hpp"

#if defined(BOOST_MSVC)
#pragma warning(push)
#pragma warning(disable:4127)   // conditional expression is constant
#pragma warning(disable:4267)   // conversion from 'size_t' to 'unsigned int',
                                // possible loss of data
#endif

namespace test
{
    template <class X>
    void check_equivalent_keys(X const& x1)
    {
        BOOST_DEDUCED_TYPENAME X::key_equal eq = x1.key_eq();
        typedef BOOST_DEDUCED_TYPENAME X::key_type key_type;
        std::set<key_type, std::less<key_type> > found_;

        BOOST_DEDUCED_TYPENAME X::const_iterator
            it = x1.begin(), end = x1.end();
        BOOST_DEDUCED_TYPENAME X::size_type size = 0;
        while(it != end) {
            // First test that the current key has not occurred before, required
            // to test either that keys are unique or that equivalent keys are
            // adjacent. (6.3.1/6)
            key_type key = get_key<X>(*it);
            if(!found_.insert(key).second)
                BOOST_ERROR("Elements with equivalent keys aren't adjacent.");

            // Iterate over equivalent keys, counting them.
            unsigned int count = 0;
            do {
                ++it;
                ++count;
                ++size;
            } while(it != end && eq(get_key<X>(*it), key));

            // If the container has unique keys, test that there's only one.
            // Since the previous test makes sure that all equivalent keys are
            // adjacent, this is all the equivalent keys - so the test is
            // sufficient. (6.3.1/6 again).
            if(test::has_unique_keys<X>::value && count != 1)
                BOOST_ERROR("Non-unique key.");

            if(x1.count(key) != count) {
                BOOST_ERROR("Incorrect output of count.");
                std::cerr<<x1.count(key)<<","<<count<<"\n";
            }

            // Check that the keys are in the correct bucket and are
            // adjacent in the bucket.
            BOOST_DEDUCED_TYPENAME X::size_type bucket = x1.bucket(key);
            BOOST_DEDUCED_TYPENAME X::const_local_iterator
                lit = x1.begin(bucket), lend = x1.end(bucket);
            for(; lit != lend && !eq(get_key<X>(*lit), key); ++lit) continue;
            if(lit == lend)
                BOOST_ERROR("Unable to find element with a local_iterator");
            unsigned int count2 = 0;
            for(; lit != lend && eq(get_key<X>(*lit), key); ++lit) ++count2;
            if(count != count2)
                BOOST_ERROR("Element count doesn't match local_iterator.");
            for(; lit != lend; ++lit) {
                if(eq(get_key<X>(*lit), key)) {
                    BOOST_ERROR("Non-adjacent element with equivalent key "
                        "in bucket.");
                    break;
                }
            }
        };

        // Check that size matches up.

        if(x1.size() != size) {
            BOOST_ERROR("x1.size() doesn't match actual size.");
            std::cout<<x1.size()<<"/"<<size<<std::endl;
        }

        // Check the load factor.

        float load_factor =
            static_cast<float>(size) / static_cast<float>(x1.bucket_count());
        using namespace std;
        if(fabs(x1.load_factor() - load_factor) > x1.load_factor() / 64)
            BOOST_ERROR("x1.load_factor() doesn't match actual load_factor.");

        // Check that size in the buckets matches up.

        BOOST_DEDUCED_TYPENAME X::size_type bucket_size = 0;

        for (BOOST_DEDUCED_TYPENAME X::size_type
                i = 0; i < x1.bucket_count(); ++i)
        {
            for (BOOST_DEDUCED_TYPENAME X::const_local_iterator
                    begin = x1.begin(i), end = x1.end(i); begin != end; ++begin)
            {
                ++bucket_size;
            }
        }

        if(x1.size() != bucket_size) {
            BOOST_ERROR("x1.size() doesn't match bucket size.");
            std::cout<<x1.size()<<"/"<<bucket_size<<std::endl;
        }
    }
}

#if defined(BOOST_MSVC)
#pragma warning(pop)
#endif

#endif

