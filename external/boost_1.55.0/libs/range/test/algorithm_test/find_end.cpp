// Boost.Range library
//
//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#include <boost/range/algorithm/find_end.hpp>

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/assign.hpp>
#include "../test_driver/range_return_test_driver.hpp"
#include <algorithm>
#include <functional>
#include <vector>
#include <set>
#include <list>

namespace boost_range_test_algorithm_find_end
{
    template<class Container2>
    class find_end_test_policy
    {
        typedef Container2 container2_t;
    public:
        explicit find_end_test_policy(const Container2& cont)
            :   m_cont(cont)
        {
        }

        container2_t cont() { return m_cont; }

        template<class Container>
        BOOST_DEDUCED_TYPENAME boost::range_iterator<Container>::type
        test_iter(Container& cont)
        {
            typedef BOOST_DEDUCED_TYPENAME boost::range_iterator<Container>::type iter_t;
            iter_t result = boost::find_end(cont, m_cont);
            BOOST_CHECK( result == boost::find_end(boost::make_iterator_range(cont), m_cont) );
            BOOST_CHECK( result == boost::find_end(cont, boost::make_iterator_range(m_cont)) );
            BOOST_CHECK( result == boost::find_end(boost::make_iterator_range(cont), boost::make_iterator_range(m_cont)) );
            return result;
        }

        template<boost::range_return_value return_type>
        struct test_range
        {
            template<class Container, class Policy>
            BOOST_DEDUCED_TYPENAME boost::range_return<Container,return_type>::type
            operator()(Policy& policy, Container& cont)
            {
                typedef BOOST_DEDUCED_TYPENAME boost::range_return<Container,return_type>::type result_t;
                result_t result = boost::find_end<return_type>(cont, policy.cont());
                BOOST_CHECK( result == boost::find_end<return_type>(boost::make_iterator_range(cont), policy.cont()) );
                BOOST_CHECK( result == boost::find_end<return_type>(cont, boost::make_iterator_range(policy.cont())) );
                BOOST_CHECK( result == boost::find_end<return_type>(boost::make_iterator_range(cont),
                                                                    boost::make_iterator_range(policy.cont())) );
                return result;
            }
        };

        template<class Container>
        BOOST_DEDUCED_TYPENAME boost::range_iterator<Container>::type
        reference(Container& cont)
        {
            return std::find_end(cont.begin(), cont.end(),
                                 m_cont.begin(), m_cont.end());
        }

    private:
        Container2 m_cont;
    };

    template<class Container2, class BinaryPredicate>
    class find_end_pred_test_policy
    {
        typedef Container2 container2_t;
    public:
        explicit find_end_pred_test_policy(const Container2& cont)
            :   m_cont(cont)
        {
        }

        container2_t& cont() { return m_cont; }
        BinaryPredicate& pred() { return m_pred; }

        template<class Container>
        BOOST_DEDUCED_TYPENAME boost::range_iterator<Container>::type
        test_iter(Container& cont)
        {
            typedef BOOST_DEDUCED_TYPENAME boost::range_iterator<Container>::type iter_t;
            iter_t it = boost::find_end(cont, m_cont, m_pred);
            BOOST_CHECK( it == boost::find_end(boost::make_iterator_range(cont), m_cont, m_pred) );
            BOOST_CHECK( it == boost::find_end(cont, boost::make_iterator_range(m_cont), m_pred) );
            BOOST_CHECK( it == boost::find_end(boost::make_iterator_range(cont), boost::make_iterator_range(m_cont), m_pred) );
            return it;
        }

        template<boost::range_return_value return_type>
        struct test_range
        {
            template<class Container, class Policy>
            BOOST_DEDUCED_TYPENAME boost::range_return<Container,return_type>::type
            operator()(Policy& policy, Container& cont)
            {
                typedef BOOST_DEDUCED_TYPENAME boost::range_return<Container,return_type>::type result_t;
                result_t result = boost::find_end<return_type>(cont, policy.cont(), policy.pred());
                BOOST_CHECK( result == boost::find_end<return_type>(boost::make_iterator_range(cont), policy.cont(), policy.pred()) );
                BOOST_CHECK( result == boost::find_end<return_type>(cont, boost::make_iterator_range(policy.cont()), policy.pred()) );
                BOOST_CHECK( result == boost::find_end<return_type>(boost::make_iterator_range(cont),
                                                                    boost::make_iterator_range(policy.cont()), policy.pred()) );
                return boost::find_end<return_type>(cont, policy.cont(), policy.pred());
            }
        };

        template<class Container>
        BOOST_DEDUCED_TYPENAME boost::range_iterator<Container>::type
        reference(Container& cont)
        {
            return std::find_end(cont.begin(), cont.end(),
                                 m_cont.begin(), m_cont.end(),
                                 m_pred);
        }

    private:
        Container2      m_cont;
        BinaryPredicate m_pred;
    };

    template<class Container1, class Container2>
    void run_tests(Container1& cont1, Container2& cont2)
    {
        boost::range_test::range_return_test_driver test_driver;
        test_driver(cont1, find_end_test_policy<Container2>(cont2));
        test_driver(cont1, find_end_pred_test_policy<Container2, std::less<int> >(cont2));
        test_driver(cont2, find_end_pred_test_policy<Container2, std::greater<int> >(cont2));
    }

    template<class Container1, class Container2>
    void test_find_end_impl()
    {
        using namespace boost::assign;

        typedef BOOST_DEDUCED_TYPENAME boost::remove_const<Container1>::type container1_t;
        typedef BOOST_DEDUCED_TYPENAME boost::remove_const<Container2>::type container2_t;

        container1_t mcont1;
        Container1& cont1 = mcont1;
        container2_t mcont2;
        Container2& cont2 = mcont2;

        run_tests(cont1, cont2);

        mcont1 += 1;
        run_tests(cont1, cont2);

        mcont2 += 1;
        run_tests(cont1, cont2);

        mcont1 += 2,3,4,5,6,7,8,9;
        mcont2 += 2,3,4;
        run_tests(cont1, cont2);

        mcont2.clear();
        mcont2 += 7,8,9;
        run_tests(cont1, cont2);
    }

    void test_find_end()
    {
        test_find_end_impl< std::vector<int>, std::vector<int> >();
        test_find_end_impl< std::list<int>, std::list<int> >();
        test_find_end_impl< std::deque<int>, std::deque<int> >();
        test_find_end_impl< const std::vector<int>, const std::vector<int> >();
        test_find_end_impl< const std::list<int>, const std::list<int> >();
        test_find_end_impl< const std::deque<int>, const std::deque<int> >();
        test_find_end_impl< const std::vector<int>, const std::list<int> >();
        test_find_end_impl< const std::list<int>, const std::vector<int> >();
        test_find_end_impl< const std::vector<int>, std::list<int> >();
        test_find_end_impl< const std::list<int>, std::vector<int> >();
        test_find_end_impl< std::vector<int>, std::list<int> >();
        test_find_end_impl< std::list<int>, std::vector<int> >();
    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.algorithm.find_end" );

    test->add( BOOST_TEST_CASE( &boost_range_test_algorithm_find_end::test_find_end ) );

    return test;
}
