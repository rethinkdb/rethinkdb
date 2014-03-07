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
#include <boost/range/adaptor/adjacent_filtered.hpp>

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/assign.hpp>
#include <algorithm>
#include <list>
#include <set>
#include <string>
#include <vector>

namespace boost
{
    namespace
    {
        template< class Container >
        void adjacent_filtered_test_impl( Container& c )
        {
            using namespace boost::adaptors;

            typedef BOOST_DEDUCED_TYPENAME Container::value_type value_t;

            // This is my preferred syntax using the | operator.
            std::vector< value_t > test_result1
                = boost::copy_range< std::vector< value_t > >(
                c | adjacent_filtered(std::not_equal_to< value_t >()));

            // This is an alternative syntax preferred by some.
            std::vector< value_t > test_result2
                = boost::copy_range< std::vector< value_t > >(
                    adaptors::adjacent_filter(c, std::not_equal_to< value_t >()));

            // Calculate the reference result.
            std::vector< value_t > reference_result;
            typedef BOOST_DEDUCED_TYPENAME Container::const_iterator iter_t;
            value_t prev_v = value_t();
            for (iter_t it = c.begin(); it != c.end(); ++it)
            {
                if (it == c.begin())
                {
                    reference_result.push_back(*it);
                }
                else if (*it != prev_v)
                {
                    reference_result.push_back(*it);
                }
                prev_v = *it;
            }

            BOOST_CHECK_EQUAL_COLLECTIONS( reference_result.begin(),
                                           reference_result.end(),
                                           test_result1.begin(),
                                           test_result1.end() );

            BOOST_CHECK_EQUAL_COLLECTIONS( reference_result.begin(),
                                           reference_result.end(),
                                           test_result2.begin(),
                                           test_result2.end() );
        }

        template< class Collection >
        void adjacent_filtered_test_impl()
        {
            using namespace boost::assign;

            Collection c;

            // test empty collection
            adjacent_filtered_test_impl(c);

            // test one element;
            c += 1;
            adjacent_filtered_test_impl(c);

            // test many elements;
            c += 1,2,2,2,3,4,4,4,4,5,6,7,8,9,9;
            adjacent_filtered_test_impl(c);
        }

        void adjacent_filtered_test()
        {
            adjacent_filtered_test_impl< std::vector< int > >();
            adjacent_filtered_test_impl< std::list< int > >();
            adjacent_filtered_test_impl< std::set< int > >();
            adjacent_filtered_test_impl< std::multiset< int > >();
        }
    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.adaptor.adjacent_filtered" );

    test->add( BOOST_TEST_CASE( &boost::adjacent_filtered_test ) );

    return test;
}
