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
#include <boost/range/algorithm/replace_copy_if.hpp>

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

namespace
{
    template< class Iterator, class Value >
    void test_append(Iterator target, Value value)
    {
        *target++ = value;
    }

    template< class Container, class UnaryPredicate >
    void test_replace_copy_if_impl( const Container& c, UnaryPredicate pred )
    {
        typedef BOOST_DEDUCED_TYPENAME boost::range_value<const Container>::type value_type;
        const value_type replace_with = value_type();
        std::vector<value_type> reference;

        test_append(
            std::replace_copy_if(c.begin(), c.end(), std::back_inserter(reference), pred, replace_with),
            value_type()
            );

        std::vector<value_type> test;
        test_append(
            boost::replace_copy_if(c, std::back_inserter(test), pred, replace_with),
            value_type()
            );

        BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                                       test.begin(), test.end() );
            
        std::vector<value_type> test2;
        test_append(
            boost::replace_copy_if(boost::make_iterator_range(c),
                                   std::back_inserter(test2), pred,
                                   replace_with),
            value_type()
            );
            
        BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                                       test2.begin(), test2.end() );
    }

    template< class Container >
    void test_replace_copy_if_( const Container& c, int to_replace )
    {
        test_replace_copy_if_impl(c, boost::bind(std::equal_to<int>(), _1, to_replace));
        test_replace_copy_if_impl(c, boost::bind(std::not_equal_to<int>(), _1, to_replace));
    }

    template< class Container >
    void test_replace_copy_if_impl()
    {
        using namespace boost::assign;

        Container cont;
        test_replace_copy_if_(cont, 0);

        cont.clear();
        cont += 1;
        test_replace_copy_if_(cont, 0);
        test_replace_copy_if_(cont, 1);

        cont.clear();
        cont += 1,1,1,1,1;
        test_replace_copy_if_(cont, 0);
        test_replace_copy_if_(cont, 1);

        cont.clear();
        cont += 1,2,3,4,5,6,7,8,9;
        test_replace_copy_if_(cont, 1);
        test_replace_copy_if_(cont, 9);
        test_replace_copy_if_(cont, 4);
    }

    inline void test_replace_copy_if()
    {
        test_replace_copy_if_impl< std::vector<int> >();
        test_replace_copy_if_impl< std::list<int> >();
        test_replace_copy_if_impl< std::deque<int> >();
    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.algorithm.replace_copy_if" );

    test->add( BOOST_TEST_CASE( &test_replace_copy_if ) );

    return test;
}
