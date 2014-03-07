// Boost.Range library
//
//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_TEST_TEST_FUNCTION_EQUAL_TO_X_HPP_INCLUDED
#define BOOST_RANGE_TEST_TEST_FUNCTION_EQUAL_TO_X_HPP_INCLUDED

namespace boost
{
    namespace range_test_function
    {
        template<class Arg>
        struct equal_to_x
        {
            typedef bool result_type;
            typedef Arg argument_type;

            explicit equal_to_x(Arg x) : m_x(x) {}
            bool operator()(Arg x) const { return x == m_x; }

        private:
            Arg m_x;
        };
    }
}

#endif // include guard
