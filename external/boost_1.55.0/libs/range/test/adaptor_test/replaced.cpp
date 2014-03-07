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
#include <boost/range/adaptor/replaced.hpp>

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/assign.hpp>
#include <boost/range/algorithm_ext.hpp>

#include <algorithm>
#include <list>
#include <set>
#include <vector>

namespace boost
{
    namespace
    {
        template< class Container >
        void replaced_test_impl( Container& c )
        {
            using namespace boost::adaptors;

            const int value_to_replace = 1;
            const int replacement_value = 0;

            std::vector< int > test_result1;
            boost::push_back(test_result1, c | replaced(value_to_replace, replacement_value));

            std::vector< int > test_result2;
            boost::push_back(test_result2, adaptors::replace(c, value_to_replace, replacement_value));

            std::vector< int > reference( c.begin(), c.end() );
            std::replace(reference.begin(), reference.end(), value_to_replace, replacement_value);

            BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                                           test_result1.begin(), test_result1.end() );
        }

        template< class Container >
        void replaced_test_impl()
        {
            using namespace boost::assign;

            Container c;

            // Test empty
            replaced_test_impl(c);

            // Test one
            c += 1;
            replaced_test_impl(c);

            // Test many
            c += 1,1,1,2,2,2,3,3,3,3,3,4,5,6,6,6,7,8,9;
            replaced_test_impl(c);
        }

        void replaced_test()
        {
            replaced_test_impl< std::vector< int > >();
            replaced_test_impl< std::list< int > >();
            replaced_test_impl< std::set< int > >();
            replaced_test_impl< std::multiset< int > >();
        }
    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.adaptor.replaced" );

    test->add( BOOST_TEST_CASE( &boost::replaced_test ) );

    return test;
}
