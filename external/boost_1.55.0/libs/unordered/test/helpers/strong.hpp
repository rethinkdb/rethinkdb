
// Copyright 2005-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_UNORDERED_TEST_HELPERS_STRONG_HEADER)
#define BOOST_UNORDERED_TEST_HELPERS_STRONG_HEADER

#include <boost/config.hpp>
#include <iterator>
#include "./equivalent.hpp"
#include "./list.hpp"
#include "./exception_test.hpp"

namespace test
{
    template <class X>
    class strong
    {
        typedef test::list<BOOST_DEDUCED_TYPENAME X::value_type> values_type;
        values_type values_;
        unsigned int allocations_;
    public:
        void store(X const& x, unsigned int allocations = 0) {
            DISABLE_EXCEPTIONS;
            values_.clear();
            values_.insert(x.cbegin(), x.cend());
            allocations_ = allocations;
        }

        void test(X const& x, unsigned int allocations = 0) const {
            if(!(x.size() == values_.size() &&
                    test::equal(x.cbegin(), x.cend(), values_.begin(),
                        test::equivalent)))
                BOOST_ERROR("Strong exception safety failure.");
            if(allocations != allocations_)
                BOOST_ERROR("Strong exception failure: extra allocations.");
        }
    };
}

#endif
