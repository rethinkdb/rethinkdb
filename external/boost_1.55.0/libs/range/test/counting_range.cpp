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
// Disable a warning from <xutility> since this noise might
// stop us detecting a problem in our code.
#include <boost/range/counting_range.hpp>

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
        template<class Container>
        void counting_range_test_impl(int first, int last)
        {
            Container reference;
            for (int i = first; i < last; ++i)
                reference.push_back(i);

            Container test;
            push_back( test, counting_range(first, last) );

            BOOST_CHECK_EQUAL_COLLECTIONS(
                reference.begin(), reference.end(),
                test.begin(), test.end());
        }

        template<class Container>
        void counting_range_test_impl()
        {
            counting_range_test_impl<Container>(0, 0);
            counting_range_test_impl<Container>(-1, -1);
            counting_range_test_impl<Container>(-1, 0);
            counting_range_test_impl<Container>(0, 1);
            counting_range_test_impl<Container>(-100, 100);
            counting_range_test_impl<Container>(50, 55);
        }
    }

    void counting_range_test()
    {
        counting_range_test_impl<std::vector<int> >();
        counting_range_test_impl<std::list<int> >();
        counting_range_test_impl<std::deque<int> >();
    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.adaptor.counting_range" );

    test->add( BOOST_TEST_CASE( &boost::counting_range_test ) );

    return test;
}
