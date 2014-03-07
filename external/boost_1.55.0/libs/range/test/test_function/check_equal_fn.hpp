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
#ifndef BOOST_RANGE_TEST_FUNCTIONS_CHECK_EQUAL_FN_HPP_INCLUDED
#define BOOST_RANGE_TEST_FUNCTIONS_CHECK_EQUAL_FN_HPP_INCLUDED

#include "counted_function.hpp"

namespace boost
{
    namespace range_test_function
    {
        template< class Collection >
        class check_equal_fn : private counted_function
        {
            typedef BOOST_DEDUCED_TYPENAME Collection::const_iterator iter_t;
        public:
            explicit check_equal_fn( const Collection& c )
                : m_it(boost::begin(c)), m_last(boost::end(c)) {}

            using counted_function::invocation_count;

            void operator()(int x) const
            {
                invoked();
                BOOST_CHECK( m_it != m_last );
                if (m_it != m_last)
                {
                    BOOST_CHECK_EQUAL( *m_it, x );
                    ++m_it;
                }
            }

        private:
            mutable iter_t m_it;
            iter_t m_last;
        };

    } // namespace range_test_function
} // namespace boost

#endif // include guard
