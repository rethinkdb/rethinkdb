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
#include <boost/range/algorithm/find.hpp>

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/assign.hpp>
#include "../test_driver/range_return_test_driver.hpp"
#include <algorithm>
#include <functional>
#include <list>
#include <numeric>
#include <deque>
#include <vector>

namespace boost_range_test_algorithm_find
{
    class find_test_policy
    {
    public:
        template<class Container>
        BOOST_DEDUCED_TYPENAME boost::range_iterator<Container>::type
        test_iter(Container& cont)
        {
            typedef BOOST_DEDUCED_TYPENAME boost::range_iterator<Container>::type iter_t;
            iter_t result = boost::find(cont, 3);
            iter_t result2 = boost::find(boost::make_iterator_range(cont), 3);
            BOOST_CHECK( result == result2 );
            return result;
        }

        template<boost::range_return_value return_type>
        struct test_range
        {
            template<class Container, class Policy>
            BOOST_DEDUCED_TYPENAME boost::range_return<Container,return_type>::type
            operator()(Policy&, Container& cont)
            {
                typedef BOOST_DEDUCED_TYPENAME boost::range_return<Container,return_type>::type result_t;
                result_t result = boost::find<return_type>(cont, 3);
                result_t result2 = boost::find<return_type>(boost::make_iterator_range(cont), 3);
                BOOST_CHECK( result == result2 );
                return result;
            }
        };

        template<class Container>
        BOOST_DEDUCED_TYPENAME boost::range_iterator<Container>::type
        reference(Container& cont)
        {
            return std::find(cont.begin(), cont.end(), 3);
        }
    };

    template<class Container>
    void test_find_container()
    {
        using namespace boost::assign;

        typedef BOOST_DEDUCED_TYPENAME boost::remove_const<Container>::type container_t;

        boost::range_test::range_return_test_driver test_driver;

        container_t mcont;
        Container& cont = mcont;
        test_driver(cont, find_test_policy());

        mcont.clear();
        mcont += 1;
        test_driver(cont, find_test_policy());

        mcont.clear();
        mcont += 1,2,3,4,5,6,7,8,9;
        test_driver(cont, find_test_policy());
    }

    void test_find()
    {
        test_find_container< std::vector<int> >();
        test_find_container< std::list<int> >();
        test_find_container< std::deque<int> >();

        test_find_container< const std::vector<int> >();
        test_find_container< const std::list<int> >();
        test_find_container< const std::deque<int> >();

        std::vector<int> vi;
        const std::vector<int>& cvi = vi;
        std::vector<int>::const_iterator it = boost::find(vi, 0);
        std::vector<int>::const_iterator it2 = boost::find(cvi, 0);
        BOOST_CHECK( it == it2 );
    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.algorithm.find" );

    test->add( BOOST_TEST_CASE( &boost_range_test_algorithm_find::test_find ) );

    return test;
}
