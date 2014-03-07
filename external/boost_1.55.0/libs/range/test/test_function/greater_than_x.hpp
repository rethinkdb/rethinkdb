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
#ifndef BOOST_RANGE_TEST_FUNCTION_GREATER_THAN_X_HPP_INCLUDED
#define BOOST_RANGE_TEST_FUNCTION_GREATER_THAN_X_HPP_INCLUDED

namespace boost
{
    namespace range_test_function
    {
        template< class Number >
        struct greater_than_x
        {
            typedef bool result_type;
            typedef Number argument_type;

            explicit greater_than_x(Number x) : m_x(x) {}
            bool operator()(Number x) const { return x > m_x; }
        private:
            Number m_x;
        };
    } // namespace range_test_function
} // namespace boost

#endif // include guard
