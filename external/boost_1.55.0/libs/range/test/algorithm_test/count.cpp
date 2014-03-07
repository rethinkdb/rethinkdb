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
#include <boost/range/algorithm/count.hpp>

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/assign.hpp>
#include <algorithm>
#include <list>
#include <set>
#include <vector>

namespace boost
{
    namespace
    {
        template< class Container >
        void test_count_impl()
        {
            using namespace boost::assign;

            Container cont;
            const Container& cref_cont = cont;

            BOOST_CHECK_EQUAL( 0u, boost::count(cont, 0u) );
            BOOST_CHECK_EQUAL( 0u, boost::count(cref_cont, 0u) );
            BOOST_CHECK_EQUAL( 0u, boost::count(boost::make_iterator_range(cont), 0u) );

            cont += 1;
            BOOST_CHECK_EQUAL( 0u, boost::count(cont, 0u) );
            BOOST_CHECK_EQUAL( 0u, boost::count(cref_cont, 0u) );
            BOOST_CHECK_EQUAL( 0u, boost::count(boost::make_iterator_range(cont), 0u) );
            BOOST_CHECK_EQUAL( 1u, boost::count(cont, 1u) );
            BOOST_CHECK_EQUAL( 1u, boost::count(cref_cont, 1u) );
            BOOST_CHECK_EQUAL( 1u, boost::count(boost::make_iterator_range(cont), 1u) );

            cont += 2,3,4,5,6,7,8,9;
            BOOST_CHECK_EQUAL( 0u, boost::count(cont, 0u) );
            BOOST_CHECK_EQUAL( 0u, boost::count(cref_cont, 0u) );
            BOOST_CHECK_EQUAL( 0u, boost::count(boost::make_iterator_range(cont), 0u) );
            BOOST_CHECK_EQUAL( 1u, boost::count(cont, 1u) );
            BOOST_CHECK_EQUAL( 1u, boost::count(cref_cont, 1u) );
            BOOST_CHECK_EQUAL( 1u, boost::count(boost::make_iterator_range(cont), 1u) );

            cont += 2;
            BOOST_CHECK_EQUAL( 0u, boost::count(cont, 0u) );
            BOOST_CHECK_EQUAL( 0u, boost::count(cref_cont, 0u) );
            BOOST_CHECK_EQUAL( 0u, boost::count(boost::make_iterator_range(cont), 0u) );
            BOOST_CHECK_EQUAL( 1u, boost::count(cont, 1u) );
            BOOST_CHECK_EQUAL( 1u, boost::count(cref_cont, 1u) );
            BOOST_CHECK_EQUAL( 1u, boost::count(boost::make_iterator_range(cont), 1u) );
            BOOST_CHECK_EQUAL( 2u, boost::count(cont, 2u) );
            BOOST_CHECK_EQUAL( 2u, boost::count(cref_cont, 2u) );
            BOOST_CHECK_EQUAL( 2u, boost::count(boost::make_iterator_range(cont), 2u) );
        }

        void test_count()
        {
            test_count_impl< std::vector<int> >();
            test_count_impl< std::list<int> >();
            test_count_impl< std::multiset<int> >();
        }
    }
}


boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.algorithm.count" );

    test->add( BOOST_TEST_CASE( &boost::test_count ) );

    return test;
}
