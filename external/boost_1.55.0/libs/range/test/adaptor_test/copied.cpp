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
#include <boost/range/adaptor/copied.hpp>

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/assign.hpp>

#include <algorithm>
#include <deque>
#include <string>
#include <vector>
#include <boost/range/algorithm_ext.hpp>

namespace boost
{
    namespace
    {
        template< class Container >
        void copied_test_impl( Container& c )
        {
            using namespace boost::adaptors;

            // This is my preferred syntax using the | operator.
            std::vector< int > test_result1;
            boost::push_back(test_result1, c | copied(0u, c.size()));

            // This is the alternative syntax preferred by some.
            std::vector< int > test_result2;
            boost::push_back(test_result2, adaptors::copy(c, 0u, c.size()));

            BOOST_CHECK_EQUAL_COLLECTIONS( test_result1.begin(), test_result1.end(),
                                           c.begin(), c.end() );

            BOOST_CHECK_EQUAL_COLLECTIONS( test_result2.begin(), test_result2.end(),
                                           c.begin(), c.end() );
        }

        template< class Container >
        void copied_test_impl()
        {
            using namespace boost::assign;

            Container c;

            // test empty collection
            copied_test_impl(c);

            // test one element
            c += 1;
            copied_test_impl(c);

            // test many elements
            c += 1,2,2,2,3,4,4,4,4,5,6,7,8,9,9;
            copied_test_impl(c);
        }

        void copied_test()
        {
            copied_test_impl< std::vector< int > >();
            copied_test_impl< std::deque< int > >();
        }
    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.adaptor.copied" );

    test->add( BOOST_TEST_CASE( &boost::copied_test ) );

    return test;
}
