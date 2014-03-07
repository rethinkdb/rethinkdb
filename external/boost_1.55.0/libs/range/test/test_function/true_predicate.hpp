// Boost.Range library
//
//  Copyright Neil Groves 2010. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_TEST_TEST_FUNCTION_TRUE_PREDICATE_HPP_INCLUDED
#define BOOST_RANGE_TEST_TEST_FUNCTION_TRUE_PREDICATE_HPP_INCLUDED

namespace boost
{
    namespace range_test_function
    {
        struct true_predicate
        {
            typedef bool result_type;

            bool operator()() const { return true; }
            template<class Arg> bool operator()(Arg) const { return true; }
            template<class Arg1, class Arg2> bool operator()(Arg1,Arg2) const { return true; }
        };
    }
}

#endif // include guard
