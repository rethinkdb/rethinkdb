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
#include <boost/range/algorithm/transform.hpp>

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/assign.hpp>
#include "../test_function/multiply_by_x.hpp"
#include <algorithm>
#include <list>
#include <set>
#include <vector>

namespace boost
{
    namespace
    {
        template< class Container >
        void test_transform_impl1(Container& cont)
        {
            using namespace boost::range_test_function;

            const Container& ccont = cont;

            typedef BOOST_DEDUCED_TYPENAME Container::value_type value_t;

            std::vector<value_t> target(cont.size());
            std::vector<value_t> reference(cont.size());
            typedef BOOST_DEDUCED_TYPENAME std::vector<value_t>::iterator iterator_t;

            multiply_by_x<int> fn(2);

            iterator_t reference_it
                = std::transform(cont.begin(), cont.end(), reference.begin(), fn);

            iterator_t test_it
                = boost::transform(cont, target.begin(), fn);

            BOOST_CHECK( test_it == target.end() );
            BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                                           target.begin(), target.end() );
                
            BOOST_CHECK( test_it == boost::transform(boost::make_iterator_range(cont), target.begin(), fn) );
            BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                                           target.begin(), target.end() );
                                           
            target.clear();
            target.resize(ccont.size());

            test_it = boost::transform(ccont, target.begin(), fn);
            
            BOOST_CHECK( test_it == target.end() );
            BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                                           target.begin(), target.end() );
            BOOST_CHECK( test_it == boost::transform(boost::make_iterator_range(ccont), target.begin(), fn) );
            BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                                           target.begin(), target.end() );
        }

        template< class Container >
        void test_transform_impl1()
        {
            using namespace boost::assign;

            Container cont;

            test_transform_impl1(cont);

            cont += 1;
            test_transform_impl1(cont);

            cont += 2,3,4,5,6,7;
            test_transform_impl1(cont);
        }

        template< class Container1, class Container2 >
        void test_transform_impl2(Container1& cont1, Container2& cont2)
        {
            const Container1& ccont1 = cont1;
            const Container2& ccont2 = cont2;

            BOOST_CHECK_EQUAL( cont1.size(), cont2.size() );

            typedef BOOST_DEDUCED_TYPENAME Container1::value_type value_t;

            std::vector<value_t> target(cont1.size());
            std::vector<value_t> reference(cont1.size());
            typedef BOOST_DEDUCED_TYPENAME std::vector<value_t>::iterator iterator_t;

            std::multiplies<int> fn;

            iterator_t reference_it
                = std::transform(cont1.begin(), cont1.end(),
                                 cont2.begin(), reference.begin(), fn);

            iterator_t test_it
                = boost::transform(cont1, cont2, target.begin(), fn);
                
            BOOST_CHECK( test_it == target.end() );
            BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                                           target.begin(), target.end() );
                                           
            BOOST_CHECK( test_it == boost::transform(boost::make_iterator_range(cont1), cont2, target.begin(), fn) );
            BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                                           target.begin(), target.end() );
                                           
            BOOST_CHECK( test_it == boost::transform(cont1, boost::make_iterator_range(cont2), target.begin(), fn) );
            BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                                           target.begin(), target.end() );
                                           
            BOOST_CHECK( test_it == boost::transform(boost::make_iterator_range(cont1),
                                                     boost::make_iterator_range(cont2),
                                                     target.begin(), fn) );
            BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                                           target.begin(), target.end() );
                         

            target.clear();
            target.resize(ccont1.size());

            test_it = boost::transform(ccont1, ccont2, target.begin(), fn);

            BOOST_CHECK( test_it == target.end() );
            BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                target.begin(), target.end() );
        }

        template< class Container1, class Container2 >
        void test_transform_impl2()
        {
            using namespace boost::assign;

            Container1 cont1;
            Container2 cont2;

            test_transform_impl2(cont1, cont2);

            cont1 += 1;
            cont2 += 2;
            test_transform_impl2(cont1, cont2);

            cont1 += 2,3,4,5,6,7;
            cont2 += 4,6,8,10,12,14;
            test_transform_impl2(cont1, cont2);
        }

        void test_transform()
        {
            test_transform_impl1< std::vector<int> >();
            test_transform_impl1< std::list<int> >();
            test_transform_impl1< std::set<int> >();
            test_transform_impl1< std::multiset<int> >();

            test_transform_impl2< std::vector<int>, std::list<int> >();
            test_transform_impl2< std::list<int>, std::vector<int> >();
            test_transform_impl2< std::set<int>, std::set<int> >();
            test_transform_impl2< std::multiset<int>, std::list<int> >();
        }
    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.algorithm.transform" );

    test->add( BOOST_TEST_CASE( &boost::test_transform ) );

    return test;
}
