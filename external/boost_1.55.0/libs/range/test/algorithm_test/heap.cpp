//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#include <boost/range/algorithm/heap_algorithm.hpp>

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
        void check_equal(const Container1& cont1, const Container2& cont2)
        {
            BOOST_CHECK_EQUAL_COLLECTIONS(
                cont1.begin(), cont1.end(),
                cont2.begin(), cont2.end()
                );
        }

        void test()
        {
            using namespace boost::assign;

            std::vector<int> reference;
            reference += 1,2,3,4,5,6,7,8,9;

            std::vector<int> test_cont(reference);
            std::vector<int> test_cont2(reference);

            std::make_heap(reference.begin(), reference.end());
            boost::make_heap(test_cont);
            check_equal(reference, test_cont);
            boost::make_heap(boost::make_iterator_range(test_cont2));
            check_equal(reference, test_cont2);

            std::push_heap(reference.begin(), reference.end());
            boost::push_heap(test_cont);
            check_equal(reference, test_cont);
            boost::push_heap(boost::make_iterator_range(test_cont2));
            check_equal(reference, test_cont2);

            std::make_heap(reference.begin(), reference.end());
            boost::make_heap(test_cont);
            boost::make_heap(boost::make_iterator_range(test_cont2));

            std::sort_heap(reference.begin(), reference.end());
            boost::sort_heap(test_cont);
            check_equal(reference, test_cont);
            boost::sort_heap(boost::make_iterator_range(test_cont2));
            check_equal(reference, test_cont2);

            std::make_heap(reference.begin(), reference.end());
            boost::make_heap(test_cont);
            boost::make_heap(boost::make_iterator_range(test_cont2));

            std::pop_heap(reference.begin(), reference.end());
            boost::pop_heap(test_cont);
            check_equal(reference, test_cont);
            boost::pop_heap(boost::make_iterator_range(test_cont2));
            check_equal(reference, test_cont2);
        }

        template<class BinaryPredicate>
        void test_pred(BinaryPredicate pred)
        {
            using namespace boost::assign;

            std::vector<int> reference;
            reference += 1,2,3,4,5,6,7,8,9;
            std::sort(reference.begin(), reference.end(), pred);

            std::vector<int> test_cont(reference);
            std::vector<int> test_cont2(reference);

            std::make_heap(reference.begin(), reference.end(), pred);
            boost::make_heap(test_cont, pred);
            check_equal(reference, test_cont);
            boost::make_heap(boost::make_iterator_range(test_cont2), pred);
            check_equal(reference, test_cont2);

            reference.push_back(5);
            test_cont.push_back(5);
            test_cont2.push_back(5);
            std::push_heap(reference.begin(), reference.end(), pred);
            boost::push_heap(test_cont, pred);
            check_equal(reference, test_cont);
            boost::push_heap(boost::make_iterator_range(test_cont2), pred);
            check_equal(reference, test_cont2);

            std::make_heap(reference.begin(), reference.end(), pred);
            boost::make_heap(test_cont, pred);
            boost::make_heap(boost::make_iterator_range(test_cont2), pred);

            std::sort_heap(reference.begin(), reference.end(), pred);
            boost::sort_heap(test_cont, pred);
            check_equal(reference, test_cont);
            boost::sort_heap(boost::make_iterator_range(test_cont2), pred);
            check_equal(reference, test_cont2);

            std::make_heap(reference.begin(), reference.end(), pred);
            boost::make_heap(test_cont, pred);
            boost::make_heap(boost::make_iterator_range(test_cont2), pred);

            std::pop_heap(reference.begin(), reference.end(), pred);
            boost::pop_heap(test_cont, pred);
            check_equal(reference, test_cont);
            boost::pop_heap(boost::make_iterator_range(test_cont2), pred);
            check_equal(reference, test_cont2);
        }

        void test_heap()
        {
            test();
            test_pred(std::less<int>());
            test_pred(std::greater<int>());
        }
    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.algorithm.heap" );

    test->add( BOOST_TEST_CASE( &boost::test_heap ) );

    return test;
}
