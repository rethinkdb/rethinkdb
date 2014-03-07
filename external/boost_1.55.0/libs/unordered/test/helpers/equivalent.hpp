
// Copyright 2005-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_UNORDERED_TESTS_EQUIVALENT_HEADER)
#define BOOST_UNORDERED_TESTS_EQUIVALENT_HEADER

#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <algorithm>
#include "./metafunctions.hpp"
#include "./fwd.hpp"
#include "./list.hpp"

namespace test
{
    template <class T1, class T2>
    bool equivalent_impl(T1 const& x, T2 const& y, base_type) {
        return x == y;
    }

    template <class T>
    bool equivalent_impl(boost::hash<T> const&, boost::hash<T> const&,
        derived_type)
    {
        return true;
    }

    template <class T>
    bool equivalent_impl(std::equal_to<T> const&, std::equal_to<T> const&,
        derived_type)
    {
        return true;
    }

    template <class T1, class T2, class T3, class T4>
    bool equivalent_impl(std::pair<T1, T2> const& x1,
            std::pair<T3, T4> const& x2, derived_type) {
        return equivalent_impl(x1.first, x2.first, derived) &&
            equivalent_impl(x1.second, x2.second, derived);
    }

    struct equivalent_type {
        template <class T1, class T2>
        bool operator()(T1 const& x, T2 const& y) {
            return equivalent_impl(x, y, derived);
        }
    };

    // This won't be a problem as I'm only using a single compile unit
    // in each test (this is actually require by the minimal test
    // framework).
    // 
    // boostinspect:nounnamed
    namespace {
        equivalent_type equivalent;
    }

    template <class Container>
    class unordered_equivalence_tester
    {
        BOOST_DEDUCED_TYPENAME Container::size_type size_;
        BOOST_DEDUCED_TYPENAME Container::hasher hasher_;
        BOOST_DEDUCED_TYPENAME Container::key_equal key_equal_;
        float max_load_factor_;

        typedef test::list<BOOST_DEDUCED_TYPENAME Container::value_type>
            value_list;
        value_list values_;
    public:
        unordered_equivalence_tester(Container const &x)
            : size_(x.size()),
            hasher_(x.hash_function()), key_equal_(x.key_eq()),
            max_load_factor_(x.max_load_factor()),
            values_(x.begin(), x.end())
        {
            values_.sort();
        }

        bool operator()(Container const& x) const
        {
            if(!((size_ == x.size()) &&
                (test::equivalent(hasher_, x.hash_function())) &&
                (test::equivalent(key_equal_, x.key_eq())) &&
                (max_load_factor_ == x.max_load_factor()) &&
                (values_.size() == x.size()))) return false;

            value_list copy(x.begin(), x.end());
            copy.sort();
            return values_ == copy;
        }
    private:
        unordered_equivalence_tester();
    };
}

#endif
