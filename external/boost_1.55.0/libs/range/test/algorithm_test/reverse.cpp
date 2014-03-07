//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#include <boost/range/algorithm/reverse.hpp>

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/assign.hpp>
#include <boost/bind.hpp>
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
        template<class Container>
        void test_reverse_impl(Container& cont)
        {
            Container reference(cont);
            Container test(cont);
            Container test2(cont);

            boost::reverse(test);
            std::reverse(reference.begin(), reference.end());
            BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                                           test.begin(), test.end() );
                                           
            boost::reverse(boost::make_iterator_range(test2));
            BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                                           test2.begin(), test2.end() );
        }

        template<class Container>
        void test_reverse_impl()
        {
            using namespace boost::assign;

            Container cont;
            test_reverse_impl(cont);

            cont.clear();
            cont += 1;
            test_reverse_impl(cont);

            cont.clear();
            cont += 1,2,3,4,5,6,7,8,9;
            test_reverse_impl(cont);
        }

        void test_reverse()
        {
            test_reverse_impl< std::vector<int> >();
            test_reverse_impl< std::list<int> >();
            test_reverse_impl< std::deque<int> >();
        }
    }
}


boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.algorithm.reverse" );

    test->add( BOOST_TEST_CASE( &boost::test_reverse ) );

    return test;
}
