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
#include <boost/range/algorithm/fill.hpp>

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/assign.hpp>
#include <algorithm>
#include <functional>
#include <list>
#include <numeric>
#include <deque>
#include <vector>

namespace boost
{
    namespace
    {
        template< class Container >
        void test_fill_impl(Container& cont)
        {
            Container reference(cont);
            std::fill(reference.begin(), reference.end(), 1);

            Container target(cont);
            boost::fill(target, 1);

            BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                target.begin(), target.end() );

            Container target2(cont);
            boost::fill(boost::make_iterator_range(target2), 1);

            BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                                           target2.begin(), target2.end() );
        }

        template< class Container >
        void test_fill_impl()
        {
            using namespace boost::assign;

            Container cont;
            test_fill_impl(cont);

            cont.clear();
            cont += 2;
            test_fill_impl(cont);

            cont.clear();
            cont += 1,2;
            test_fill_impl(cont);

            cont.clear();
            cont += 1,2,3,4,5,6,7,8,9;
            test_fill_impl(cont);
        }

        void test_fill()
        {
            test_fill_impl< std::vector<int> >();
            test_fill_impl< std::list<int> >();
            test_fill_impl< std::deque<int> >();
        }
    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.algorithm.fill" );

    test->add( BOOST_TEST_CASE( &boost::test_fill ) );

    return test;
}
