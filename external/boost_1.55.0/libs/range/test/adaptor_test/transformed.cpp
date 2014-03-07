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
#include <boost/range/adaptor/transformed.hpp>

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/assign.hpp>
#include <boost/range/algorithm_ext.hpp>

#include <algorithm>
#include <list>
#include <set>
#include <vector>

namespace boost
{
    namespace
    {
        struct double_x
        {
            typedef int result_type;
            int operator()(int x) const { return x * 2; }
        };

        struct halve_x
        {
            typedef int result_type;
            int operator()(int x) const { return x / 2; }
        };

        template< class Container, class TransformFn >
        void transformed_test_impl_core( Container& c, TransformFn fn )
        {
            using namespace boost::adaptors;

            std::vector< int > test_result1;
            boost::push_back(test_result1, c | transformed(fn));

            std::vector< int > test_result2;
            boost::push_back(test_result2, adaptors::transform(c, fn));

            std::vector< int > reference;
            std::transform(c.begin(), c.end(), std::back_inserter(reference), fn);

            BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                                           test_result1.begin(), test_result1.end() );

            BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                                           test_result2.begin(), test_result2.end() );
        }

        template< class Container, class TransformFn >
        void transformed_test_fn_impl()
        {
            using namespace boost::assign;

            Container c;
            TransformFn fn;

            // Test empty
            transformed_test_impl_core(c, fn);

            // Test one element
            c += 1;
            transformed_test_impl_core(c, fn);

            // Test many elements
            c += 1,1,1,2,2,2,2,2,3,4,5,6,7,8,9;
            transformed_test_impl_core(c, fn);
        }

        template< class Container >
        void transformed_test_impl()
        {
            transformed_test_fn_impl< Container, double_x >();
            transformed_test_fn_impl< Container, halve_x >();
        }

        void transformed_test()
        {
            transformed_test_impl< std::vector< int > >();
            transformed_test_impl< std::list< int > >();
            transformed_test_impl< std::set< int > >();
            transformed_test_impl< std::multiset< int > >();
        }
    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.adaptor.transformed" );

    test->add( BOOST_TEST_CASE( &boost::transformed_test ) );

    return test;
}
