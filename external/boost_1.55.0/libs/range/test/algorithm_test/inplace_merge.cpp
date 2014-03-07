//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#include <boost/range/algorithm/inplace_merge.hpp>

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
            typedef BOOST_DEDUCED_TYPENAME Container1::value_type value_t;
            typedef BOOST_DEDUCED_TYPENAME std::vector<value_t>::iterator iterator_t;

            std::vector<value_t> reference_target(cont1.begin(), cont1.end());
            reference_target.insert(reference_target.end(),
                                    cont2.begin(), cont2.end());

            std::vector<value_t> test_target(reference_target);
            std::vector<value_t> test_target2(reference_target);

            std::inplace_merge(reference_target.begin(),
                               reference_target.begin() + cont1.size(),
                               reference_target.end());

            boost::inplace_merge(test_target,
                                 test_target.begin() + cont1.size());

            BOOST_CHECK_EQUAL_COLLECTIONS(
                reference_target.begin(), reference_target.end(),
                test_target.begin(), test_target.end()
                );
                
            boost::inplace_merge(boost::make_iterator_range(test_target2),
                                 test_target2.begin() + cont1.size());
                                 
            BOOST_CHECK_EQUAL_COLLECTIONS(
                reference_target.begin(), reference_target.end(),
                test_target2.begin(), test_target2.end()
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

        template<class Container1,
                 class Container2,
                 class BinaryPredicate>
        void test_pred(Container1 cont1, Container2 cont2, BinaryPredicate pred)
        {
            typedef BOOST_DEDUCED_TYPENAME Container1::value_type value_t;
            typedef BOOST_DEDUCED_TYPENAME std::vector<value_t>::iterator iterator_t;

            sort_container(cont1, pred);
            sort_container(cont2, pred);

            std::vector<value_t> reference_target(cont1.begin(), cont1.end());
            reference_target.insert(reference_target.end(),
                                    cont2.begin(), cont2.end());

            std::vector<value_t> test_target(reference_target);
            std::vector<value_t> test_target2(reference_target);

            std::inplace_merge(reference_target.begin(),
                               reference_target.begin() + cont1.size(),
                               reference_target.end(), pred);

            boost::inplace_merge(test_target,
                                 test_target.begin() + cont1.size(),
                                 pred);

            BOOST_CHECK_EQUAL_COLLECTIONS(
                reference_target.begin(), reference_target.end(),
                test_target.begin(), test_target.end()
                );

            boost::inplace_merge(boost::make_iterator_range(test_target2),
                                 test_target2.begin() + cont1.size(),
                                 pred);
                                 
            BOOST_CHECK_EQUAL_COLLECTIONS(
                reference_target.begin(), reference_target.end(),
                test_target2.begin(), test_target2.end()
                );
        }

        template<class Container1, class Container2>
        void test_inplace_merge_impl(Container1& cont1, Container2& cont2)
        {
            test(cont1, cont2);
            test_pred(cont1, cont2, std::less<int>());
            test_pred(cont1, cont2, std::greater<int>());
        }

        template<class Container1, class Container2>
        void test_inplace_merge_impl()
        {
            using namespace boost::assign;

            Container1 cont1;
            Container2 cont2;

            test_inplace_merge_impl(cont1, cont2);

            cont1.clear();
            cont2.clear();
            cont1 += 1;
            test_inplace_merge_impl(cont1, cont2);

            cont1.clear();
            cont2.clear();
            cont2 += 1;
            test_inplace_merge_impl(cont1, cont2);

            cont1.clear();
            cont2.clear();
            cont1 += 1,3,5,7,9,11,13,15,17,19;
            cont2 += 2,4,6,8,10,12,14,16,18,20;
            test_inplace_merge_impl(cont1, cont2);
        }

        void test_inplace_merge()
        {
            test_inplace_merge_impl< std::vector<int>, std::vector<int> >();
        }
    }
}


boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.algorithm.inplace_merge" );

    test->add( BOOST_TEST_CASE( &boost::test_inplace_merge ) );

    return test;
}
