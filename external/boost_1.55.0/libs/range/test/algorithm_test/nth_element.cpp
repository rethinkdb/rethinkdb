//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#include <boost/range/algorithm/nth_element.hpp>

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
        struct nth_element_policy
        {
            template<class Container, class Iterator>
            void test_nth_element(Container& cont, Iterator mid)
            {
                const Container old_cont(cont);
                
                boost::nth_element(cont, mid);
                
                // Test the same operation on the container, for the
                // case where a temporary is passed as the first
                // argument.
                Container cont2(old_cont);
                const std::size_t index = std::distance(cont.begin(), mid);
                Iterator mid2(cont2.begin());
                std::advance(mid2, index);
                boost::nth_element(boost::make_iterator_range(cont2), mid2);
                
                BOOST_CHECK_EQUAL_COLLECTIONS( cont.begin(), cont.end(),
                                               cont2.begin(), cont2.end() );
            }

            template<class Container, class Iterator>
            void reference_nth_element(Container& cont, Iterator mid)
            {
                std::nth_element(cont.begin(), mid, cont.end());
            }
        };

        template<class BinaryPredicate>
        struct nth_element_pred_policy
        {
            template<class Container, class Iterator>
            void test_nth_element(Container& cont, Iterator mid)
            {
                const Container old_cont(cont);
                
                boost::nth_element(cont, mid, BinaryPredicate());
                
                Container cont2(old_cont);
                const std::size_t index = std::distance(cont.begin(), mid);
                Iterator mid2(cont2.begin());
                std::advance(mid2, index);
                boost::nth_element(boost::make_iterator_range(cont2), mid2,
                                   BinaryPredicate());
                                   
                BOOST_CHECK_EQUAL_COLLECTIONS( cont.begin(), cont.end(),
                                               cont2.begin(), cont2.end() );
            }

            template<class Container, class Iterator>
            void reference_nth_element(Container& cont, Iterator mid)
            {
                std::nth_element(cont.begin(), mid, cont.end(), BinaryPredicate());
            }
        };

        template<class Container, class TestPolicy>
        void test_nth_element_tp_impl(Container& cont, TestPolicy policy)
        {
            Container reference(cont);
            Container test(cont);

            typedef BOOST_DEDUCED_TYPENAME range_iterator<Container>::type iterator_t;

            BOOST_CHECK_EQUAL( reference.size(), test.size() );
            if (reference.size() != test.size())
                return;

            iterator_t reference_mid = reference.begin();
            iterator_t test_mid = test.begin();

            bool complete = false;
            while (!complete)
            {
                if (reference_mid == reference.end())
                    complete = true;

                policy.test_nth_element(test, test_mid);
                policy.reference_nth_element(reference, reference_mid);

                BOOST_CHECK_EQUAL_COLLECTIONS(
                    reference.begin(), reference.end(),
                    test.begin(), test.end()
                    );

                if (reference_mid != reference.end())
                {
                    ++reference_mid;
                    ++test_mid;
                }
            }
        }

        template<class Container>
        void test_nth_element_impl(Container& cont)
        {
            test_nth_element_tp_impl(cont, nth_element_policy());
        }

        template<class Container, class BinaryPredicate>
        void test_nth_element_impl(Container& cont, BinaryPredicate pred)
        {
            test_nth_element_tp_impl(cont, nth_element_pred_policy<BinaryPredicate>());
        }

        template<class Container>
        void run_tests(Container& cont)
        {
            test_nth_element_impl(cont);
            test_nth_element_impl(cont, std::less<int>());
            test_nth_element_impl(cont, std::greater<int>());
        }

        template<class Container>
        void test_nth_element_impl()
        {
            using namespace boost::assign;

            Container cont;
            run_tests(cont);

            cont.clear();
            cont += 1;
            run_tests(cont);

            cont.clear();
            cont += 1,2,3,4,5,6,7,8,9;
            run_tests(cont);
        }

        void test_nth_element()
        {
            test_nth_element_impl< std::vector<int> >();
            test_nth_element_impl< std::deque<int> >();
        }
    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.algorithm.nth_element" );

    test->add( BOOST_TEST_CASE( &boost::test_nth_element ) );

    return test;
}
