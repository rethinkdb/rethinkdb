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
#include <boost/range/algorithm/replace.hpp>

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/assign.hpp>
#include <algorithm>
#include <list>
#include <deque>
#include <vector>

namespace boost
{
    namespace
    {
        template< class Container >
        void test_replace_impl(Container& cont)
        {
            const int what = 2;
            const int with_what = 5;

            std::vector<int> reference(cont.begin(), cont.end());
            std::replace(reference.begin(), reference.end(), what, with_what);

            std::vector<int> target(cont.begin(), cont.end());
            boost::replace(target, what, with_what);

            BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                                           target.begin(), target.end() );
                
            std::vector<int> target2(cont.begin(), cont.end());
            boost::replace(boost::make_iterator_range(target2), what,
                           with_what);
                           
            BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                                           target2.begin(), target2.end() );

        }

        template< class Container >
        void test_replace_impl()
        {
            using namespace boost::assign;

            Container cont;
            test_replace_impl(cont);

            cont.clear();
            cont += 1;
            test_replace_impl(cont);

            cont.clear();
            cont += 1,2,3,4,5,6,7,8,9;
            test_replace_impl(cont);
        }

        void test_replace()
        {
            test_replace_impl< std::vector<int> >();
            test_replace_impl< std::list<int> >();
            test_replace_impl< std::deque<int> >();
        }
    }
}


boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.algorithm.replace" );

    test->add( BOOST_TEST_CASE( &boost::test_replace ) );

    return test;
}
