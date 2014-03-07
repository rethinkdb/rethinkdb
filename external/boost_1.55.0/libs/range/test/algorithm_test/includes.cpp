//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#include <boost/range/algorithm/set_algorithm.hpp>

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
        template<class Container1, class Container2>
        void test(Container1& cont1, Container2& cont2)
        {
            Container1 old_cont1(cont1);
            Container2 old_cont2(cont2);

            bool reference_result
                = std::includes(cont1.begin(), cont1.end(),
                                cont2.begin(), cont2.end());

            bool test_result = boost::includes(cont1, cont2);

            BOOST_CHECK( reference_result == test_result );

            BOOST_CHECK_EQUAL_COLLECTIONS(
                old_cont1.begin(), old_cont1.end(),
                cont1.begin(), cont1.end()
                );

            BOOST_CHECK_EQUAL_COLLECTIONS(
                old_cont2.begin(), old_cont2.end(),
                cont2.begin(), cont2.end()
                );
                
            BOOST_CHECK( test_result == boost::includes(boost::make_iterator_range(cont1), cont2) );
            BOOST_CHECK( test_result == boost::includes(cont1, boost::make_iterator_range(cont2)) );
            BOOST_CHECK( test_result == boost::includes(boost::make_iterator_range(cont1), boost::make_iterator_range(cont2)) );
        }

        template<class Container, class BinaryPredicate>
        void sort_container(Container& cont, BinaryPredicate pred)
        {
            typedef BOOST_DEDUCED_TYPENAME Container::value_type value_t;

            std::vector<value_t> temp(cont.begin(), cont.end());
            std::sort(temp.begin(), temp.end(), pred);
            cont.assign(temp.begin(), temp.end());
        }

        template<class Container1,
                 class Container2,
                 class BinaryPredicate>
        void test_pred(Container1 cont1, Container2 cont2,
                       BinaryPredicate pred)
        {
            sort_container(cont1, pred);
            sort_container(cont2, pred);

            Container1 old_cont1(cont1);
            Container2 old_cont2(cont2);

            bool reference_result
                = std::includes(cont1.begin(), cont1.end(),
                                cont2.begin(), cont2.end(),
                                pred);

            bool test_result = boost::includes(cont1, cont2, pred);

            BOOST_CHECK( reference_result == test_result );

            BOOST_CHECK_EQUAL_COLLECTIONS(
                old_cont1.begin(), old_cont1.end(),
                cont1.begin(), cont1.end()
                );

            BOOST_CHECK_EQUAL_COLLECTIONS(
                old_cont2.begin(), old_cont2.end(),
                cont2.begin(), cont2.end()
                );
                
            BOOST_CHECK( test_result == boost::includes(boost::make_iterator_range(cont1), cont2, pred) );
            BOOST_CHECK( test_result == boost::includes(cont1, boost::make_iterator_range(cont2), pred) );
            BOOST_CHECK( test_result == boost::includes(boost::make_iterator_range(cont1), boost::make_iterator_range(cont2), pred) );
        }

        template<class Container1, class Container2>
        void test_includes_impl(
            Container1& cont1,
            Container2& cont2
            )
        {
            test(cont1, cont2);
            test_pred(cont1, cont2, std::less<int>());
            test_pred(cont1, cont2, std::greater<int>());
        }

        template<class Container1, class Container2>
        void test_includes_impl()
        {
            using namespace boost::assign;

            Container1 cont1;
            Container2 cont2;

            test_includes_impl(cont1, cont2);

            cont1.clear();
            cont2.clear();
            cont1 += 1;
            test_includes_impl(cont1, cont2);

            cont1.clear();
            cont2.clear();
            cont2 += 1;
            test_includes_impl(cont1, cont2);

            cont1.clear();
            cont2.clear();
            cont1 += 1,2,3,4,5,6,7,8,9;
            cont2 += 2,3,4;
            test_includes_impl(cont1, cont2);

            cont1.clear();
            cont2.clear();
            cont1 += 2,3,4;
            cont2 += 1,2,3,4,5,6,7,8,9;
            test_includes_impl(cont1, cont2);
        }

        void test_includes()
        {
            test_includes_impl< std::vector<int>, std::vector<int> >();
            test_includes_impl< std::list<int>, std::list<int> >();
            test_includes_impl< std::deque<int>, std::deque<int> >();
            test_includes_impl< std::vector<int>, std::list<int> >();
            test_includes_impl< std::list<int>, std::vector<int> >();
        }
    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.algorithm.includes" );

    test->add( BOOST_TEST_CASE( &boost::test_includes ) );

    return test;
}
