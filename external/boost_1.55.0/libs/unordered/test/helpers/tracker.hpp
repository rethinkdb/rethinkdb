
// Copyright 2006-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This header contains metafunctions/functions to get the equivalent
// associative container for an unordered container, and compare the contents.

#if !defined(BOOST_UNORDERED_TEST_HELPERS_TRACKER_HEADER)
#define BOOST_UNORDERED_TEST_HELPERS_TRACKER_HEADER

#include <set>
#include <map>
#include <iterator>
#include <algorithm>
#include <boost/type_traits/is_same.hpp>
#include "../objects/fwd.hpp"
#include "./metafunctions.hpp"
#include "./helpers.hpp"
#include "./equivalent.hpp"
#include "./list.hpp"

namespace test
{
    template <typename X>
    struct equals_to_compare
    {
        typedef std::less<BOOST_DEDUCED_TYPENAME X::first_argument_type>
            type;
    };

    template <>
    struct equals_to_compare<test::equal_to>
    {
        typedef test::less type;
    };

    template <class X1, class X2>
    void compare_range(X1 const& x1, X2 const& x2)
    {
        typedef test::list<BOOST_DEDUCED_TYPENAME X1::value_type> value_list;
        value_list values1(x1.begin(), x1.end());
        value_list values2(x2.begin(), x2.end());
        values1.sort();
        values2.sort();
        BOOST_TEST(values1.size() == values2.size() &&
                test::equal(values1.begin(), values1.end(), values2.begin(),
                    test::equivalent));
    }

    template <class X1, class X2, class T>
    void compare_pairs(X1 const& x1, X2 const& x2, T*)
    {
        test::list<T> values1(x1.first, x1.second);
        test::list<T> values2(x2.first, x2.second);
        values1.sort();
        values2.sort();
        BOOST_TEST(values1.size() == values2.size() &&
                test::equal(values1.begin(), values1.end(),
                    values2.begin(), test::equivalent));
    }

    template <typename X>
    struct ordered_base;

    template <class V, class H, class P, class A>
    struct ordered_base<boost::unordered_set<V, H, P, A> >
    {
        typedef std::set<V,
            BOOST_DEDUCED_TYPENAME equals_to_compare<P>::type>
            type;
    };

    template <class V, class H, class P, class A>
    struct ordered_base<boost::unordered_multiset<V, H, P, A> >
    {
        typedef std::multiset<V,
            BOOST_DEDUCED_TYPENAME equals_to_compare<P>::type>
            type;
    };

    template <class K, class M, class H, class P, class A>
    struct ordered_base<boost::unordered_map<K, M, H, P, A> >
    {
        typedef std::map<K, M,
            BOOST_DEDUCED_TYPENAME equals_to_compare<P>::type>
            type;
    };

    template <class K, class M, class H, class P, class A>
    struct ordered_base<boost::unordered_multimap<K, M, H, P, A> >
    {
        typedef std::multimap<K, M,
            BOOST_DEDUCED_TYPENAME equals_to_compare<P>::type>
            type;
    };

    template <class X>
    class ordered : public ordered_base<X>::type
    {
        typedef BOOST_DEDUCED_TYPENAME ordered_base<X>::type base;
    public:
        typedef BOOST_DEDUCED_TYPENAME base::key_compare key_compare;

        ordered()
            : base()
        {}

        explicit ordered(key_compare const& kc)
            : base(kc)
        {}

        void compare(X const& x)
        {
            compare_range(x, *this);
        }

        void compare_key(X const& x,
            BOOST_DEDUCED_TYPENAME X::value_type const& val)
        {
            compare_pairs(
                x.equal_range(get_key<X>(val)),
                this->equal_range(get_key<X>(val)),
                (BOOST_DEDUCED_TYPENAME X::value_type*) 0);
        }

        template <class It>
        void insert_range(It b, It e) {
            while(b != e) {
                this->insert(*b);
                ++b;
            }
        }
    };

    template <class Equals>
    BOOST_DEDUCED_TYPENAME
        equals_to_compare<Equals>::type create_compare(Equals const&)
    {
        BOOST_DEDUCED_TYPENAME equals_to_compare<Equals>::type x;
        return x;
    }

    template <class X>
    ordered<X> create_ordered(X const& container)
    {
        return ordered<X>(create_compare(container.key_eq()));
    }

    template <class X1, class X2>
    void check_container(X1 const& container, X2 const& values)
    {
        ordered<X1> tracker = create_ordered(container);
        tracker.insert_range(values.begin(), values.end());
        tracker.compare(container);
    }
}

#endif

