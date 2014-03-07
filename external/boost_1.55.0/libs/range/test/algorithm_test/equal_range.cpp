//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#include <boost/range/algorithm/equal_range.hpp>

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
        template<class Container, class Pair>
        void check_result(
            const Container& reference,
            Pair             reference_pair,
            const Container& test,
            Pair             test_pair
            )
        {
            typedef BOOST_DEDUCED_TYPENAME range_iterator<const Container>::type
                const_iterator_t;

            BOOST_CHECK_EQUAL_COLLECTIONS(
                reference.begin(), reference.end(),
                test.begin(), test.end()
                );

            BOOST_CHECK_EQUAL(
                std::distance<const_iterator_t>(reference.begin(), reference_pair.first),
                std::distance<const_iterator_t>(test.begin(), test_pair.first)
                );

            BOOST_CHECK_EQUAL(
                std::distance<const_iterator_t>(reference.begin(), reference_pair.second),
                std::distance<const_iterator_t>(test.begin(), test_pair.second)
                );

            BOOST_CHECK_EQUAL_COLLECTIONS(
                reference_pair.first, reference_pair.second,
                test_pair.first, test_pair.second
                );
        }

        template<class Container>
        void test(Container& cont)
        {
            Container reference(cont);
            Container test(cont);

            typedef BOOST_DEDUCED_TYPENAME range_iterator<Container>::type iterator_t;
            typedef std::pair<iterator_t, iterator_t> pair_t;

            pair_t reference_result
                = std::equal_range(reference.begin(), reference.end(), 5);

            pair_t test_result = boost::equal_range(test, 5);

            check_result(reference, reference_result, test, test_result);

            pair_t test_result2 = boost::equal_range(boost::make_iterator_range(test), 5);

            check_result(reference, reference_result, test, test_result2);
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
            typedef BOOST_DEDUCED_TYPENAME boost::remove_const<Container>::type container_t;

            container_t reference_temp(cont);
            container_t test_temp(cont);

            sort_container(reference_temp, pred);
            sort_container(test_temp, pred);

            Container reference(reference_temp);
            Container test(test_temp);

            typedef BOOST_DEDUCED_TYPENAME range_iterator<Container>::type iterator_t;
            typedef std::pair<iterator_t, iterator_t> pair_t;

            pair_t reference_result
                = std::equal_range(reference.begin(), reference.end(), 5,
                                    BinaryPredicate());

            pair_t test_result = boost::equal_range(test, 5, BinaryPredicate());

            check_result(reference, reference_result, test, test_result);

            pair_t test_result2 = boost::equal_range(boost::make_iterator_range(test), 5, BinaryPredicate());

            check_result(reference, reference_result, test, test_result2);
        }

        template<class Container>
        void test_driver(const Container& cont)
        {
            Container mutable_cont(cont);
            test(mutable_cont);

            test(cont);
        }

        template<class Container, class BinaryPredicate>
        void test_pred_driver(const Container& cont, BinaryPredicate pred)
        {
            Container mutable_cont(cont);
            test_pred(mutable_cont, pred);

            test_pred(cont, pred);
        }

        template<class Container>
        void test_equal_range_impl()
        {
            using namespace boost::assign;

            Container cont;

            test_driver(cont);
            test_pred_driver(cont, std::less<int>());
            test_pred_driver(cont, std::greater<int>());

            cont.clear();
            cont += 1;
            test_driver(cont);
            test_pred_driver(cont, std::less<int>());
            test_pred_driver(cont, std::greater<int>());

            cont.clear();
            cont += 1,2,3,4,5,6,7,8,9;
            test_driver(cont);
            test_pred_driver(cont, std::less<int>());
            test_pred_driver(cont, std::greater<int>());
        }

        void test_equal_range()
        {
            test_equal_range_impl< std::vector<int> >();
            test_equal_range_impl< std::list<int> >();
            test_equal_range_impl< std::deque<int> >();
        }
    }
}


boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.algorithm.equal_range" );

    test->add( BOOST_TEST_CASE( &boost::test_equal_range ) );

    return test;
}
