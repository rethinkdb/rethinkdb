// Boost.Range library
//
//  Copyright Neil Groves 2010. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#include <boost/range/iterator_range.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/array.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <vector>

namespace
{
    void test_pointer_as_iterator()
    {
        boost::array<int,3> arr;
        boost::iterator_range<const int*> r(arr.begin(), arr.end());
        r[0];
    }
} // anonymous namespace

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.pointer_as_iterator" );

    test->add(BOOST_TEST_CASE( &test_pointer_as_iterator ));

    return test;
}
