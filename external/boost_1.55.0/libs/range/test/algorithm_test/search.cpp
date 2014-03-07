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
#include <boost/range/algorithm/search.hpp>

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/assign.hpp>
#include <algorithm>
#include <list>
#include <set>
#include <vector>

namespace boost
{
    namespace
    {
        template< class Container1, class Container2 >
        void test_search_impl(Container1& cont1, Container2& cont2)
        {
            typedef BOOST_DEDUCED_TYPENAME Container1::const_iterator const_iterator1_t;
            typedef BOOST_DEDUCED_TYPENAME Container1::iterator iterator1_t;

            const Container1& ccont1 = cont1;
            const Container2& ccont2 = cont2;

            iterator1_t it = boost::search(cont1, cont2);
            BOOST_CHECK( it == boost::search(boost::make_iterator_range(cont1), cont2) );
            BOOST_CHECK( it == boost::search(cont1, boost::make_iterator_range(cont2)) );
            BOOST_CHECK( it == boost::search(boost::make_iterator_range(cont1),
                                             boost::make_iterator_range(cont2)) );
            iterator1_t it2 = boost::search(cont1, ccont2);
            BOOST_CHECK( it2 == boost::search(boost::make_iterator_range(cont1), ccont2) );
            BOOST_CHECK( it2 == boost::search(cont1, boost::make_iterator_range(ccont2)) );
            BOOST_CHECK( it2 == boost::search(boost::make_iterator_range(cont1),
                                              boost::make_iterator_range(ccont2)) );
            const_iterator1_t cit = boost::search(ccont1, cont2);
            BOOST_CHECK( cit == boost::search(boost::make_iterator_range(ccont1), cont2) );
            BOOST_CHECK( cit == boost::search(ccont1, boost::make_iterator_range(cont2)) );
            BOOST_CHECK( cit == boost::search(boost::make_iterator_range(ccont1),
                                              boost::make_iterator_range(cont2)) );
            const_iterator1_t cit2 = boost::search(ccont1, ccont2);
            BOOST_CHECK( cit2 == boost::search(boost::make_iterator_range(ccont1), ccont2) );
            BOOST_CHECK( cit2 == boost::search(ccont1, boost::make_iterator_range(ccont2)) );
            BOOST_CHECK( cit2 == boost::search(boost::make_iterator_range(ccont1),
                                               boost::make_iterator_range(ccont2)) );

            BOOST_CHECK( it == std::search(cont1.begin(), cont1.end(), cont2.begin(), cont2.end()) );
            BOOST_CHECK( it2 == std::search(cont1.begin(), cont1.end(), ccont2.begin(), ccont2.end()) );
            BOOST_CHECK( cit == std::search(ccont1.begin(), ccont1.end(), cont2.begin(), cont2.end()) );
            BOOST_CHECK( cit2 == std::search(ccont1.begin(), ccont1.end(), ccont2.begin(), ccont2.end()) );
        }

        template< class Container1, class Container2 >
        void test_search_impl()
        {
            using namespace boost::assign;

            Container1 cont1;
            Container2 cont2;

            test_search_impl(cont1, cont2);

            cont1 += 1;
            test_search_impl(cont1, cont2);

            cont1.clear();
            cont2 += 1;
            test_search_impl(cont1, cont2);

            cont1 += 1;
            test_search_impl(cont1, cont2);

            cont1.clear();
            cont2.clear();
            cont1 += 1,2,3,4,5,6,7,8,9;
            cont2 += 10,11,12;
            test_search_impl(cont1, cont2);

            cont2.clear();
            cont2 += 4,5,6;
            test_search_impl(cont1, cont2);
        }

        void test_search()
        {
            test_search_impl< std::list<int>, std::list<int> >();
            test_search_impl< std::vector<int>, std::vector<int> >();
            test_search_impl< std::set<int>, std::set<int> >();
            test_search_impl< std::list<int>, std::vector<int> >();
            test_search_impl< std::vector<int>, std::list<int> >();
        }
    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.algorithm.search" );

    test->add( BOOST_TEST_CASE( &boost::test_search ) );

    return test;
}
