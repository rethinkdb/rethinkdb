//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#include <boost/range/algorithm/rotate.hpp>

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
        template<class Container, class Iterator>
        void test_rotate_impl(Container& cont, Iterator where_it)
        {
            Container reference(cont);
            Container test(cont);

            Iterator reference_where_it = reference.begin();
            std::advance(reference_where_it,
                std::distance(cont.begin(), where_it));

            std::rotate(reference.begin(), reference_where_it, reference.end());

            Iterator test_where_it = test.begin();
            std::advance(test_where_it,
                std::distance(cont.begin(), where_it));

            boost::rotate(test, test_where_it);

            BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                                           test.begin(), test.end() );
                
            test = cont;
            test_where_it = test.begin();
            std::advance(test_where_it,
                         std::distance(cont.begin(), where_it));
                         
            boost::rotate(boost::make_iterator_range(test), test_where_it);
            
            BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                                           test.begin(), test.end() );
        }

        template<class Container>
        void test_rotate_impl(Container& cont)
        {
            typedef BOOST_DEDUCED_TYPENAME range_iterator<Container>::type iterator_t;

            iterator_t last = cont.end();
            for (iterator_t it = cont.begin(); it != last; ++it)
            {
                test_rotate_impl(cont, it);
            }
        }

        template<class Container>
        void test_rotate_impl()
        {
            using namespace boost::assign;

            Container cont;
            test_rotate_impl(cont);

            cont.clear();
            cont += 1;
            test_rotate_impl(cont);

            cont.clear();
            cont += 1,2,3,4,5,6,7,8,9;
            test_rotate_impl(cont);
        }

        void test_rotate()
        {
            test_rotate_impl< std::vector<int> >();
            test_rotate_impl< std::list<int> >();
            test_rotate_impl< std::deque<int> >();
        }
    }
}


boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.algorithm.rotate" );

    test->add( BOOST_TEST_CASE( &boost::test_rotate ) );

    return test;
}
