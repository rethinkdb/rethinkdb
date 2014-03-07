// Boost.Range library
//
//  Copyright Neil Groves 2011. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#include <boost/range/irange.hpp>
#include <boost/range/algorithm_ext/push_back.hpp>

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <vector>

namespace boost
{
    namespace
    {
        void test_irange_termination()
        {
            std::vector<int> reference;
            for (int i = 0; i < 9; i += 2)
                reference.push_back(i);
                
            std::vector<int> actual;
            boost::push_back(actual, boost::irange(0,9,2));
            
            BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(),
                                          reference.begin(), reference.end());
        }
    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.ticket_5544" );

    test->add( BOOST_TEST_CASE( &boost::test_irange_termination ) );

    return test;
}
