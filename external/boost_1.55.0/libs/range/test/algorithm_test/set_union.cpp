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
        template<class Container1, class Iterator, class Container2>
        void check_result(
            Container1& reference,
            Iterator    reference_result,
            Container2& test_cont,
            Iterator    test_result
            )
        {
            BOOST_CHECK_EQUAL(
                std::distance<Iterator>(reference.begin(), reference_result),
                std::distance<Iterator>(test_cont.begin(), test_result)
                );

            BOOST_CHECK_EQUAL_COLLECTIONS(
                reference.begin(), reference.end(),
                test_cont.begin(), test_cont.end()
                );
        }

        template<class Container1, class Container2>
        void test(Container1& cont1, Container2& cont2)
        {
            typedef BOOST_DEDUCED_TYPENAME Container1::value_type value_t;
            typedef BOOST_DEDUCED_TYPENAME std::vector<value_t>::iterator iterator_t;

            std::vector<value_t> reference(cont1.size() + cont2.size());
            std::vector<value_t> test_cont(reference);

            iterator_t reference_result
                = std::set_union(cont1.begin(), cont1.end(),
                                 cont2.begin(), cont2.end(),
                                 reference.begin());

            iterator_t test_result
                = boost::set_union(cont1, cont2, test_cont.begin());

            check_result(reference, reference_result,
                         test_cont, test_result);
                         
            test_result = boost::set_union(boost::make_iterator_range(cont1),
                                           cont2, test_cont.begin());
                                           
            check_result(reference, reference_result,
                         test_cont, test_result);
                         
            test_result = boost::set_union(cont1,
                                           boost::make_iterator_range(cont2),
                                           test_cont.begin());
                                           
            check_result(reference, reference_result,
                         test_cont, test_result);
                         
            test_result = boost::set_union(boost::make_iterator_range(cont1),
                                           boost::make_iterator_range(cont2),
                                           test_cont.begin());
                                           
            check_result(reference, reference_result,
                         test_cont, test_result);
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
            typedef BOOST_DEDUCED_TYPENAME Container1::value_type value_t;
            typedef BOOST_DEDUCED_TYPENAME std::vector<value_t>::iterator iterator_t;

            sort_container(cont1, pred);
            sort_container(cont2, pred);

            std::vector<value_t> reference(cont1.size() + cont2.size());
            std::vector<value_t> test_cont(reference);

            iterator_t reference_result
                = std::set_union(cont1.begin(), cont1.end(),
                                 cont2.begin(), cont2.end(),
                                 reference.begin(),
                                 pred);

            iterator_t test_result
                = boost::set_union(cont1, cont2, test_cont.begin(), pred);

            check_result(reference, reference_result,
                         test_cont, test_result);
                         
            test_result = boost::set_union(boost::make_iterator_range(cont1),
                                           cont2, test_cont.begin(), pred);
                                           
            check_result(reference, reference_result,
                         test_cont, test_result);
                         
            test_result = boost::set_union(cont1,
                                           boost::make_iterator_range(cont2),
                                           test_cont.begin(), pred);
                                           
            check_result(reference, reference_result,
                         test_cont, test_result);
                         
            test_result = boost::set_union(boost::make_iterator_range(cont1),
                                           boost::make_iterator_range(cont2),
                                           test_cont.begin(), pred);
                                           
            check_result(reference, reference_result,
                         test_cont, test_result);
        }

        template<class Container1, class Container2>
        void test_set_union_impl(
            Container1& cont1,
            Container2& cont2
            )
        {
            test(cont1, cont2);
            test_pred(cont1, cont2, std::less<int>());
            test_pred(cont1, cont2, std::greater<int>());
        }

        template<class Container1, class Container2>
        void test_set_union_impl()
        {
            using namespace boost::assign;

            Container1 cont1;
            Container2 cont2;

            test_set_union_impl(cont1, cont2);

            cont1.clear();
            cont2.clear();
            cont1 += 1;
            test_set_union_impl(cont1, cont2);

            cont1.clear();
            cont2.clear();
            cont2 += 1;
            test_set_union_impl(cont1, cont2);

            cont1.clear();
            cont2.clear();
            cont1 += 1,2,3,4,5,6,7,8,9;
            cont2 += 2,3,4;
            test_set_union_impl(cont1, cont2);

            cont1.clear();
            cont2.clear();
            cont1 += 2,3,4;
            cont2 += 1,2,3,4,5,6,7,8,9;
            test_set_union_impl(cont1, cont2);
        }

        void test_set_union()
        {
            test_set_union_impl< std::vector<int>, std::vector<int> >();
            test_set_union_impl< std::list<int>, std::list<int> >();
            test_set_union_impl< std::deque<int>, std::deque<int> >();
            test_set_union_impl< std::vector<int>, std::list<int> >();
            test_set_union_impl< std::list<int>, std::vector<int> >();
        }
    }
}


boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.algorithm.set_union" );

    test->add( BOOST_TEST_CASE( &boost::test_set_union ) );

    return test;
}
