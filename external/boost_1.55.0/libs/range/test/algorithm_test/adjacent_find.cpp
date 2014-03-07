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
#include <boost/range/algorithm/adjacent_find.hpp>
#include <boost/range/iterator_range.hpp>
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
        template< class Container >
        void test_adjacent_find_impl()
        {
            using namespace boost::assign;

            typedef BOOST_DEDUCED_TYPENAME Container::iterator iterator_t;
            typedef BOOST_DEDUCED_TYPENAME Container::const_iterator const_iterator_t;

            Container cont;
            const Container& cref_cont = cont;

            std::equal_to<int> pred;

            BOOST_CHECK( boost::adjacent_find(cont) == cont.end() );
            BOOST_CHECK( boost::adjacent_find(cref_cont) == cref_cont.end() );
            BOOST_CHECK( boost::adjacent_find(boost::make_iterator_range(cont)) == cont.end() );
            BOOST_CHECK( boost::adjacent_find(cont, pred) == cont.end() );
            BOOST_CHECK( boost::adjacent_find(cref_cont, pred) == cref_cont.end() );
            BOOST_CHECK( boost::adjacent_find(boost::make_iterator_range(cont), pred) == cont.end() );

            cont += 1;
            BOOST_CHECK( boost::adjacent_find(cont) == cont.end() );
            BOOST_CHECK( boost::adjacent_find(cref_cont) == cref_cont.end() );
            BOOST_CHECK( boost::adjacent_find(boost::make_iterator_range(cont)) == cont.end() );
            BOOST_CHECK( boost::adjacent_find(cont, pred) == cont.end() );
            BOOST_CHECK( boost::adjacent_find(cref_cont, pred) == cref_cont.end() );
            BOOST_CHECK( boost::adjacent_find(boost::make_iterator_range(cont), pred) == cont.end() );

            cont += 2,3,4,5,5,5,6,7,8,9;
            iterator_t it = boost::adjacent_find(cont);
            iterator_t it_pred = boost::adjacent_find(cont, pred);
            BOOST_CHECK( it == it_pred );
            BOOST_CHECK( it != cont.end() );
            BOOST_CHECK( it == std::adjacent_find(cont.begin(), cont.end()) );
            if (it != cont.end())
            {
                BOOST_CHECK( *it == 5 );
            }
            BOOST_CHECK( it == boost::adjacent_find(boost::make_iterator_range(cont)) );
            BOOST_CHECK( it_pred == boost::adjacent_find(boost::make_iterator_range(cont), pred) );
            const_iterator_t cit = boost::adjacent_find(cref_cont);
            const_iterator_t cit_pred = boost::adjacent_find(cref_cont, pred);
            BOOST_CHECK( cit == cit_pred );
            BOOST_CHECK( cit != cref_cont.end() );
            BOOST_CHECK( cit == std::adjacent_find(cref_cont.begin(), cref_cont.end()) );
            if (cit != cref_cont.end())
            {
                BOOST_CHECK( *cit == 5 );
            }
        }

        void test_adjacent_find()
        {
            test_adjacent_find_impl< std::vector<int> >();
            test_adjacent_find_impl< std::list<int> >();
            test_adjacent_find_impl< std::multiset<int> >();
        }
    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.algorithm.adjacent_find" );

    test->add( BOOST_TEST_CASE( &boost::test_adjacent_find ) );

    return test;
}
