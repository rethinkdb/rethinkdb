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
#include <boost/range/algorithm_ext/insert.hpp>

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/range/iterator.hpp>
#include <boost/range/irange.hpp>
#include <algorithm>
#include <list>
#include <vector>

namespace
{
    template< class Container >
    void test_insert_impl( int n )
    {
        Container test;
        boost::insert( test, test.end(), boost::irange(0, n) );

        Container reference;
        for (int i = 0; i < n; ++i)
            reference.push_back(i);

        BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
            test.begin(), test.end() );

        // Do it again so that we are inserting into a non-empty target
        boost::insert( test, test.end(), boost::irange(0, n) );

        for (int j = 0; j < n; ++j)
            reference.push_back(j);

        BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
            test.begin(), test.end() );
    }

    template< class Container >
    void test_insert_impl()
    {
        test_insert_impl< Container >(0);
        test_insert_impl< Container >(1);
        test_insert_impl< Container >(2);
        test_insert_impl< Container >(100);
    }

    void test_insert()
    {
        test_insert_impl< std::vector<std::size_t> >();
        test_insert_impl< std::list<std::size_t> >();
    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.algorithm_ext.insert" );

    test->add( BOOST_TEST_CASE( &test_insert ) );

    return test;
}
