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
#include <boost/range/algorithm_ext/is_sorted.hpp>

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/range/iterator.hpp>
#include <algorithm>
#include <list>
#include <vector>

namespace
{
    template< class Container >
    void test_is_sorted_impl()
    {
        Container ascending;
        Container descending;

        // Empty ranges are regarded as sorted against any predicate.
        BOOST_CHECK( boost::is_sorted(ascending) );
        BOOST_CHECK( boost::is_sorted(ascending, std::less<std::size_t>()) );
        BOOST_CHECK( boost::is_sorted(ascending, std::greater<std::size_t>()) );

        for (std::size_t i = 0; i < 10; ++i)
        {
            ascending.push_back(i);
            descending.push_back(9 - i);
        }

        BOOST_CHECK( boost::is_sorted(ascending) );
        BOOST_CHECK( !boost::is_sorted(descending) );
        BOOST_CHECK( !boost::is_sorted(ascending, std::greater<std::size_t>()) );
        BOOST_CHECK( boost::is_sorted(descending, std::greater<std::size_t>()) );
    }

    void test_is_sorted()
    {
        test_is_sorted_impl< std::vector<std::size_t> >();
        test_is_sorted_impl< std::list<std::size_t> >();
    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.algorithm_ext.is_sorted" );

    test->add( BOOST_TEST_CASE( &test_is_sorted ) );

    return test;
}
