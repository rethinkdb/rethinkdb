
// Copyright 2005-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_UNORDERED_TEST_HELPERS_METAFUNCTIONS_HEADER)
#define BOOST_UNORDERED_TEST_HELPERS_METAFUNCTIONS_HEADER

#include <boost/config.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>

namespace test
{
    template <class Container>
    struct is_set
        : public boost::is_same<
            BOOST_DEDUCED_TYPENAME Container::key_type,
            BOOST_DEDUCED_TYPENAME Container::value_type> {};

    template <class Container>
    struct has_unique_keys
    {
        BOOST_STATIC_CONSTANT(bool, value = false);
    };

    template <class V, class H, class P, class A>
    struct has_unique_keys<boost::unordered_set<V, H, P, A> >
    {
        BOOST_STATIC_CONSTANT(bool, value = true);
    };

    template <class K, class M, class H, class P, class A>
    struct has_unique_keys<boost::unordered_map<K, M, H, P, A> >
    {
        BOOST_STATIC_CONSTANT(bool, value = true);
    };
}

#endif

