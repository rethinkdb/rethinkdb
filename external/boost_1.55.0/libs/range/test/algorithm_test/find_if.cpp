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
#include <boost/range/algorithm/find_if.hpp>

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/assign.hpp>
#include "../test_driver/range_return_test_driver.hpp"
#include "../test_function/greater_than_x.hpp"
#include "../test_function/false_predicate.hpp"
#include <algorithm>
#include <functional>
#include <deque>
#include <list>
#include <vector>

namespace boost_range_test_algorithm_find_if
{
    template<class UnaryPredicate>
    class find_if_test_policy
    {
    public:
        explicit find_if_test_policy(UnaryPredicate pred)
            : m_pred(pred) {}

        template<class Container>
        BOOST_DEDUCED_TYPENAME boost::range_iterator<Container>::type
        test_iter(Container& cont)
        {
            typedef BOOST_DEDUCED_TYPENAME boost::range_iterator<Container>::type iter_t;
            iter_t result = boost::find_if(cont, m_pred);
            BOOST_CHECK( result == boost::find_if(boost::make_iterator_range(cont), m_pred) );
            return result;
        }

        template<boost::range_return_value return_type>
        struct test_range
        {
            template<class Container>
            BOOST_DEDUCED_TYPENAME boost::range_return<Container,return_type>::type
            operator()(find_if_test_policy& policy, Container& cont)
            {
                typedef BOOST_DEDUCED_TYPENAME boost::range_return<Container,return_type>::type result_t;
                result_t result = boost::find_if<return_type>(cont, policy.pred());
                BOOST_CHECK( result == boost::find_if<return_type>(boost::make_iterator_range(cont), policy.pred()) );
                return result;
            }
        };

        template<class Container>
        BOOST_DEDUCED_TYPENAME boost::range_iterator<Container>::type
        reference(Container& cont)
        {
            return std::find_if(cont.begin(), cont.end(), m_pred);
        }

        UnaryPredicate& pred() { return m_pred; }

    private:
        UnaryPredicate m_pred;
    };

    template<class UnaryPredicate>
    find_if_test_policy<UnaryPredicate>
    make_policy(UnaryPredicate pred)
    {
        return find_if_test_policy<UnaryPredicate>(pred);
    }

    template<class Container>
    void test_find_if_container()
    {
        using namespace boost::assign;
        using namespace boost::range_test_function;

        typedef BOOST_DEDUCED_TYPENAME boost::remove_const<Container>::type container_t;

        boost::range_test::range_return_test_driver test_driver;

        container_t mcont;
        Container& cont = mcont;
        test_driver(cont, make_policy(greater_than_x<int>(5)));
        test_driver(cont, make_policy(false_predicate()));

        mcont.clear();
        mcont += 1;
        test_driver(cont, make_policy(greater_than_x<int>(5)));
        test_driver(cont, make_policy(false_predicate()));

        mcont.clear();
        mcont += 1,2,3,4,5,6,7,8,9;
        test_driver(cont, make_policy(greater_than_x<int>(5)));
        test_driver(cont, make_policy(false_predicate()));
    }

    void test_find_if()
    {
        test_find_if_container< std::vector<int> >();
        test_find_if_container< std::list<int> >();
        test_find_if_container< std::deque<int> >();

        test_find_if_container< const std::vector<int> >();
        test_find_if_container< const std::list<int> >();
        test_find_if_container< const std::deque<int> >();
    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.algorithm.find_if" );

    test->add( BOOST_TEST_CASE( &boost_range_test_algorithm_find_if::test_find_if ) );

    return test;
}
