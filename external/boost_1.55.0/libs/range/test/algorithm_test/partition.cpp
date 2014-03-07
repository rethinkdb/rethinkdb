//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#include <boost/range/algorithm/partition.hpp>

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/assign.hpp>
#include <boost/bind.hpp>
#include "../test_driver/range_return_test_driver.hpp"
#include <algorithm>
#include <functional>
#include <list>
#include <numeric>
#include <deque>
#include <vector>

namespace boost_range_test_algorithm_partition
{
    struct equal_to_5
    {
        typedef bool result_type;
        typedef int argument_type;
        bool operator()(int x) const { return x == 5; }
    };

    // test the 'partition' algorithm
    template<class UnaryPredicate>
    class partition_test_policy
    {
    public:
        template< class Container >
        BOOST_DEDUCED_TYPENAME boost::range_iterator<Container>::type
        test_iter(Container& cont)
        {
            typedef BOOST_DEDUCED_TYPENAME boost::range_iterator<Container>::type iter_t;

            const Container old_cont(cont);
            Container cont2(old_cont);
            iter_t result = boost::partition(cont, UnaryPredicate());

            boost::partition(cont2, UnaryPredicate());
            cont2 = old_cont;
            boost::partition(
                boost::make_iterator_range(cont2), UnaryPredicate());

            BOOST_CHECK_EQUAL_COLLECTIONS( cont.begin(), cont.end(),
                                           cont2.begin(), cont2.end() );

            return result;
        }

        UnaryPredicate pred() const { return UnaryPredicate(); }

        template< boost::range_return_value return_type >
        struct test_range
        {
            template< class Container, class Policy >
            BOOST_DEDUCED_TYPENAME boost::range_return<Container,return_type>::type
            operator()(Policy& policy, Container& cont)
            {
                typedef BOOST_DEDUCED_TYPENAME boost::range_return<Container,return_type>::type result_t;

                const Container old_cont(cont);
                Container cont2(old_cont);
                result_t result = boost::partition<return_type>(cont, policy.pred());

                // Test that operation a temporary created by using
                // make_iterator_range.
                boost::partition<return_type>(
                    boost::make_iterator_range(cont2), policy.pred());

                BOOST_CHECK_EQUAL_COLLECTIONS( cont.begin(), cont.end(),
                                               cont2.begin(), cont2.end() );

                return result;
            }
        };

        template< class Container >
        BOOST_DEDUCED_TYPENAME boost::range_iterator<Container>::type
        reference(Container& cont)
        {
            return std::partition(cont.begin(), cont.end(), UnaryPredicate());
        }
    };

    template<class Container>
    void test_partition_impl()
    {
        using namespace boost::assign;

        boost::range_test::range_return_test_driver test_driver;

        partition_test_policy< equal_to_5 > policy;

        Container cont;
        test_driver(cont, policy);

        cont.clear();
        cont += 1;
        test_driver(cont, policy);

        cont.clear();
        cont += 1,2,2,2,2,2,3,4,5,6,7,8,9;
        test_driver(cont, policy);

        cont.clear();
        cont += 1,2,2,2,2,2,3,3,3,3,4,4,4,4,4,4,4,5,6,7,8,9;
        test_driver(cont, policy);
    }

    void test_partition()
    {
        test_partition_impl< std::vector<int> >();
        test_partition_impl< std::list<int> >();
        test_partition_impl< std::deque<int> >();
    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.algorithm.partition" );

    test->add( BOOST_TEST_CASE( &boost_range_test_algorithm_partition::test_partition ) );

    return test;
}
