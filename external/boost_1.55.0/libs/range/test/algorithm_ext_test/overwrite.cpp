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
#include <boost/range/algorithm_ext/overwrite.hpp>

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/range/iterator.hpp>
#include <algorithm>
#include <list>
#include <vector>

namespace
{
    template< class Container >
    void test_overwrite_impl(std::size_t n)
    {
        Container overwrite_source;
        for (std::size_t i = 0; i < n; ++i)
            overwrite_source.push_back(i);

        Container reference;
        reference.resize(n);
        std::copy(overwrite_source.begin(), overwrite_source.end(), reference.begin());

        Container test;
        test.resize(n);
        boost::overwrite(overwrite_source, test);

        BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
            test.begin(), test.end() );

        test.clear();
        test.resize(n);
        const Container& const_overwrite_source = overwrite_source;
        boost::overwrite(const_overwrite_source, test);

        BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
            test.begin(), test.end() );
    }

    template< class Container >
    void test_overwrite_impl()
    {
        test_overwrite_impl<Container>(0);
        test_overwrite_impl<Container>(1);
        test_overwrite_impl<Container>(10);
    }

    void test_overwrite()
    {
        test_overwrite_impl< std::vector<std::size_t> >();
        test_overwrite_impl< std::list<std::size_t> >();
    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.algorithm_ext.overwrite" );

    test->add( BOOST_TEST_CASE( &test_overwrite ) );

    return test;
}
