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
#include <boost/range/has_range_iterator.hpp>

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <vector>

namespace
{
    class MockClassWithoutIterators {};

    template<typename Container>
    void test_has_range_iterator_impl(const bool expected_value)
    {
        BOOST_CHECK_EQUAL( boost::has_range_iterator<Container>::value, expected_value );
    }

    template<typename Container>
    void test_has_range_const_iterator_impl(const bool expected_value)
    {
        BOOST_CHECK_EQUAL( boost::has_range_const_iterator<Container>::value, expected_value );
    }

    void test_has_range_iterator()
    {
        test_has_range_iterator_impl< std::vector<int> >(true);
        test_has_range_iterator_impl< MockClassWithoutIterators >(false);
    }

    void test_has_range_const_iterator()
    {
        test_has_range_const_iterator_impl< std::vector<int> >(true);
        test_has_range_const_iterator_impl< MockClassWithoutIterators >(false);
    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.has_range_iterator" );

    test->add(BOOST_TEST_CASE(&test_has_range_iterator));
    test->add(BOOST_TEST_CASE(&test_has_range_const_iterator));

    return test;
}
