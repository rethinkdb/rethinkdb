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
#ifndef BOOST_RANGE_TEST_FUNCTION_COUNTED_FUNCTION_HPP_INCLUDED
#define BOOST_RANGE_TEST_FUNCTION_COUNTED_FUNCTION_HPP_INCLUDED

namespace boost
{
    namespace range_test_function
    {

        class counted_function
        {
        public:
            counted_function() : m_count(0u) {}

            void invoked() const
            {
                ++m_count;
            }

            // Return the number of times that this function object
            // has been invoked.
            unsigned int invocation_count() const { return m_count; }

        private:
            mutable unsigned int m_count;
        };

    }
}

#endif // include guard
