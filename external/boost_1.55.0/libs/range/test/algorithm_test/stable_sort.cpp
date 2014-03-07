//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#include <boost/range/algorithm/stable_sort.hpp>

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
        void test_stable_sort_impl(Container& cont)
        {
            Container reference(cont);
            Container test(cont);

            boost::stable_sort(test);
            std::stable_sort(reference.begin(), reference.end());

            BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                                           test.begin(), test.end() );
                
            test = cont;
            boost::stable_sort(boost::make_iterator_range(test));
            BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                                           test.begin(), test.end() );
        }

        template<class Container, class BinaryPredicate>
        void test_stable_sort_impl(Container& cont, BinaryPredicate pred)
        {
            Container reference(cont);
            Container test(cont);

            boost::stable_sort(test, pred);
            std::stable_sort(reference.begin(), reference.end(), pred);

            BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                                           test.begin(), test.end() );
                                           
            test = cont;
            boost::stable_sort(boost::make_iterator_range(test), pred);
            BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                                           test.begin(), test.end() );
        }

        template<class Container>
        void test_stable_sort_impl()
        {
            using namespace boost::assign;

            Container cont;
            test_stable_sort_impl(cont);
            test_stable_sort_impl(cont, std::less<int>());
            test_stable_sort_impl(cont, std::greater<int>());

            cont.clear();
            cont += 1;
            test_stable_sort_impl(cont);
            test_stable_sort_impl(cont, std::less<int>());
            test_stable_sort_impl(cont, std::greater<int>());

            cont.clear();
            cont += 1,2,3,4,5,6,7,8,9;
            test_stable_sort_impl(cont);
            test_stable_sort_impl(cont, std::less<int>());
            test_stable_sort_impl(cont, std::greater<int>());
        }

        void test_stable_sort()
        {
            test_stable_sort_impl< std::vector<int> >();
            test_stable_sort_impl< std::deque<int> >();
        }
    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.algorithm.stable_sort" );

    test->add( BOOST_TEST_CASE( &boost::test_stable_sort ) );

    return test;
}
