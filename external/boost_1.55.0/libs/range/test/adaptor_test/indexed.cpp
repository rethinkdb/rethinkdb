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
#include <boost/range/adaptor/indexed.hpp>

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/assign.hpp>
#include <boost/range/algorithm_ext.hpp>
#include <boost/range/concepts.hpp>

#include <algorithm>
#include <list>
#include <vector>

#include "../test_utils.hpp"

namespace boost
{
    namespace
    {
        template< class Container >
        void indexed_test_impl( Container& c )
        {
            using namespace boost::adaptors;

            typedef BOOST_DEDUCED_TYPENAME Container::value_type value_t;

            // This is my preferred syntax using the | operator.
            std::vector< value_t > test_result1;
            boost::push_back(test_result1, c | indexed(0));

            // This is an alternative syntax preferred by some.
            std::vector< value_t > test_result2;
            boost::push_back(test_result2, adaptors::index(c, 0));

            BOOST_CHECK_EQUAL_COLLECTIONS( c.begin(), c.end(),
                                           test_result1.begin(), test_result1.end() );

            BOOST_CHECK_EQUAL_COLLECTIONS( c.begin(), c.end(),
                                           test_result2.begin(), test_result2.end() );

            boost::indexed_range< Container > test_result3
                = c | indexed(0);

            typedef BOOST_DEDUCED_TYPENAME boost::range_const_iterator<
                boost::indexed_range< Container > >::type iter_t;

            iter_t it = test_result3.begin();
            for (std::size_t i = 0, count = c.size(); i < count; ++i)
            {
                BOOST_CHECK_EQUAL( i, static_cast<std::size_t>(it.index()) );
                ++it;
            }

        }

        template< class Container >
        void indexed_test_impl()
        {
            using namespace boost::assign;

            Container c;

            // test empty container
            indexed_test_impl(c);

            // test one element
            c += 1;
            indexed_test_impl(c);

            // test many elements
            c += 1,2,2,2,3,4,4,4,4,5,6,7,8,9,9;
            indexed_test_impl(c);
        }

        void indexed_test()
        {
            indexed_test_impl< std::vector< int > >();
            indexed_test_impl< std::list< int > >();
            
            check_random_access_range_concept(std::vector<int>() | boost::adaptors::indexed(0));
            check_bidirectional_range_concept(std::list<int>() | boost::adaptors::indexed(0));
        }
    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.adaptor.indexed" );

    test->add( BOOST_TEST_CASE( &boost::indexed_test ) );

    return test;
}
