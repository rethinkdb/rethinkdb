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
#include <boost/range/algorithm_ext/iota.hpp>

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/range/iterator.hpp>
#include <algorithm>
#include <list>
#include <vector>

namespace
{
    template< class Container >
    void test_iota_impl(std::size_t n)
    {
        Container test;
        test.resize( n );
        boost::iota( test, n );

        Container reference;
        reference.resize( n );
        std::size_t i = n;
        typedef BOOST_DEDUCED_TYPENAME Container::iterator iterator_t;
        iterator_t last = reference.end();
        for (iterator_t it = reference.begin(); it != last; ++it, ++i)
        {
            *it = i;
        }

        BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
            test.begin(), test.end() );

    }

    template< class Container >
    void test_iota_impl()
    {
        test_iota_impl< Container >(0);
        test_iota_impl< Container >(1);
        test_iota_impl< Container >(2);
        test_iota_impl< Container >(100);
    }

    void test_iota()
    {
        test_iota_impl< std::vector<std::size_t> >();
        test_iota_impl< std::list<std::size_t> >();
    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.algorithm_ext.iota" );

    test->add( BOOST_TEST_CASE( &test_iota ) );

    return test;
}
