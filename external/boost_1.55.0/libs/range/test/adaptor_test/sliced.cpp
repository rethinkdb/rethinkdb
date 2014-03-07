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
#include <boost/range/adaptor/sliced.hpp>

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/assign.hpp>
#include <boost/range/algorithm_ext.hpp>

#include <algorithm>
#include <deque>
#include <vector>

namespace boost
{
    namespace
    {
        template< class Container >
        void sliced_test_impl( Container& c )
        {
            using namespace boost::adaptors;

            typedef BOOST_DEDUCED_TYPENAME Container::value_type value_t;

            std::vector< value_t > test_result1;
            boost::push_back(test_result1, c | sliced(0u,c.size()));

            BOOST_CHECK_EQUAL_COLLECTIONS( test_result1.begin(), test_result1.end(),
                                           c.begin(), c.end() );

            std::vector< value_t > test_alt_result1;
            boost::push_back(test_alt_result1, adaptors::slice(c, 0u, c.size()));
            BOOST_CHECK_EQUAL_COLLECTIONS( test_alt_result1.begin(), test_alt_result1.end(),
                                           c.begin(), c.end() );

            BOOST_CHECK( boost::empty(c | sliced(0u, 0u)) );
            
            const std::size_t half_count = c.size() / 2u;
            if (half_count > 0u)
            {
                std::vector< value_t > test_result2;
                boost::push_back(test_result2, c | sliced(0u, half_count));

                BOOST_CHECK_EQUAL_COLLECTIONS( test_result2.begin(), test_result2.end(),
                                               c.begin(), c.begin() + half_count );

                std::vector< value_t > test_alt_result2;
                boost::push_back(test_alt_result2, adaptors::slice(c, 0u, half_count));
                BOOST_CHECK_EQUAL_COLLECTIONS( test_alt_result2.begin(), test_alt_result2.end(),
                                               c.begin(), c.begin() + half_count );
            }
        }

        template< class Container >
        void sliced_test_impl()
        {
            using namespace boost::assign;

            Container c;

            // Test empty
            sliced_test_impl(c);

            // Test one element
            c += 1;
            sliced_test_impl(c);

            // Test many elements
            c += 1,1,1,2,2,3,4,5,6,6,6,7,8,9;
            sliced_test_impl(c);
        }

        void sliced_test()
        {
            sliced_test_impl< std::vector< int > >();
            sliced_test_impl< std::deque< int > >();
        }
    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.adaptor.sliced" );

    test->add( BOOST_TEST_CASE( &boost::sliced_test ) );

    return test;
}
