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
#include <boost/range/algorithm/for_each.hpp>

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/array.hpp>
#include <boost/assign.hpp>
#include <boost/range/algorithm.hpp>

#include <list>
#include <set>
#include <vector>
#include "../test_function/check_equal_fn.hpp"

namespace boost
{
    namespace
    {
        template< class Range >
        unsigned int udistance(Range& rng)
        {
            return static_cast<unsigned int>(boost::distance(rng));
        }

        template< class SinglePassRange >
        void test_for_each_impl( SinglePassRange rng )
        {
            using namespace boost::range_test_function;

            typedef check_equal_fn< SinglePassRange > fn_t;

            // Test the mutable version
            fn_t result_fn = boost::for_each(rng, fn_t(rng));
            BOOST_CHECK_EQUAL( boost::udistance(rng), result_fn.invocation_count() );
            
            fn_t result_fn2 = boost::for_each(boost::make_iterator_range(rng), fn_t(rng));
            BOOST_CHECK_EQUAL( boost::udistance(rng), result_fn2.invocation_count() );

            // Test the constant version
            const SinglePassRange& cref_rng = rng;
            result_fn = boost::for_each(cref_rng, fn_t(cref_rng));
            BOOST_CHECK_EQUAL( boost::udistance(cref_rng), result_fn.invocation_count() );
        }

        template< class Container >
        void test_for_each_t()
        {
            using namespace boost::assign;

            // Test empty
            Container cont;
            test_for_each_impl(cont);

            // Test one element
            cont += 0;
            test_for_each_impl(cont);

            // Test many elements
            cont += 1,2,3,4,5,6,7,8,9;
            test_for_each_impl(cont);
        }

        void test_for_each()
        {
            boost::array<int, 10> a = {{ 0,1,2,3,4,5,6,7,8,9 }};
            test_for_each_impl(a);

            test_for_each_t< std::vector<int> >();
            test_for_each_t< std::list<int> >();
            test_for_each_t< std::set<int> >();
            test_for_each_t< std::multiset<int> >();
        }
    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.algorithm.for_each" );

    test->add( BOOST_TEST_CASE( &boost::test_for_each ) );

    return test;
}
