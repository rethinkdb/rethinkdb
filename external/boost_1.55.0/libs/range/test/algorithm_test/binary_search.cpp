//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#include <boost/range/algorithm/binary_search.hpp>

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
        void test(Container& cont)
        {
            Container reference(cont);
            Container test(cont);

            bool reference_result
                = std::binary_search(reference.begin(), reference.end(), 5);

            bool test_result = boost::binary_search(test, 5);

            BOOST_CHECK( reference_result == test_result );

            BOOST_CHECK( test_result == boost::binary_search(boost::make_iterator_range(test), 5) );

            BOOST_CHECK_EQUAL_COLLECTIONS(
                reference.begin(), reference.end(),
                test.begin(), test.end()
                );
        }

        template<class Container, class BinaryPredicate>
        void sort_container(Container& cont, BinaryPredicate pred)
        {
            typedef BOOST_DEDUCED_TYPENAME Container::value_type value_t;

            std::vector<value_t> temp(cont.begin(), cont.end());
            std::sort(temp.begin(), temp.end(), pred);
            cont.assign(temp.begin(), temp.end());
        }

        template<class Container, class BinaryPredicate>
        void test_pred(Container& cont, BinaryPredicate pred)
        {
            Container reference(cont);
            Container test(cont);

            sort_container(reference, pred);
            sort_container(test, pred);

            bool reference_result
                = std::binary_search(reference.begin(), reference.end(), 5,
                                        pred);

            bool test_result = boost::binary_search(test, 5, pred);

            BOOST_CHECK( test_result == boost::binary_search(boost::make_iterator_range(test), 5, pred) );

            BOOST_CHECK( reference_result == test_result );

            BOOST_CHECK_EQUAL_COLLECTIONS(
                reference.begin(), reference.end(),
                test.begin(), test.end()
                );
        }

        template<class Container>
        void test_binary_search_impl()
        {
            using namespace boost::assign;

            Container cont;

            test(cont);
            test_pred(cont, std::less<int>());
            test_pred(cont, std::greater<int>());

            cont.clear();
            cont += 1;
            test(cont);
            test_pred(cont, std::less<int>());
            test_pred(cont, std::greater<int>());

            cont.clear();
            cont += 1,2,3,4,5,6,7,8,9;
            test(cont);
            test_pred(cont, std::less<int>());
            test_pred(cont, std::greater<int>());
        }

        void test_binary_search()
        {
            test_binary_search_impl< std::vector<int> >();
            test_binary_search_impl< std::list<int> >();
            test_binary_search_impl< std::deque<int> >();
        }
    }
}


boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.algorithm.binary_search" );

    test->add( BOOST_TEST_CASE( &boost::test_binary_search ) );

    return test;
}
