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
#include <boost/range/adaptor/filtered.hpp>

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/assign.hpp>
#include <boost/range/algorithm_ext.hpp>

#include <algorithm>
#include <list>
#include <set>
#include <string>
#include <vector>

namespace boost
{
    namespace
    {
        struct always_false_pred
        {
            template< class T1 >
            bool operator()(T1) const { return false; }
        };

        struct always_true_pred
        {
            template< class T1 >
            bool operator()(T1) const { return true; }
        };

        struct is_even
        {
            template< class IntegerT >
            bool operator()( IntegerT x ) const { return x % 2 == 0; }
        };

        struct is_odd
        {
            template< class IntegerT >
            bool operator()( IntegerT x ) const { return x % 2 != 0; }
        };

        template< class Container, class Pred >
        void filtered_test_impl( Container& c, Pred pred )
        {
            using namespace boost::adaptors;

            typedef BOOST_DEDUCED_TYPENAME Container::value_type value_t;

            // This is my preferred syntax using the | operator.
            std::vector< value_t > test_result1;
            boost::push_back(test_result1, c | filtered(pred));

            // This is an alternative syntax preferred by some.
            std::vector< value_t > test_result2;
            boost::push_back(test_result2, adaptors::filter(c, pred));

            // Calculate the reference result.
            std::vector< value_t > reference_result;
            typedef BOOST_DEDUCED_TYPENAME Container::const_iterator iter_t;
            for (iter_t it = c.begin(); it != c.end(); ++it)
            {
                if (pred(*it))
                    reference_result.push_back(*it);
            }

            BOOST_CHECK_EQUAL_COLLECTIONS( reference_result.begin(),
                                           reference_result.end(),
                                           test_result1.begin(),
                                           test_result1.end() );

            BOOST_CHECK_EQUAL_COLLECTIONS( reference_result.begin(),
                                           reference_result.end(),
                                           test_result2.begin(),
                                           test_result2.end() );
        }

        template< class Container, class Pred >
        void filtered_test_impl()
        {
            using namespace boost::assign;

            Container c;

            // test empty container
            filtered_test_impl(c, Pred());

            // test one element
            c += 1;
            filtered_test_impl(c, Pred());

            // test many elements
            c += 1,2,2,2,3,4,4,4,4,5,6,7,8,9,9;
            filtered_test_impl(c, Pred());
        }

        template< class Container >
        void filtered_test_all_predicates()
        {
            filtered_test_impl< Container, always_false_pred >();
            filtered_test_impl< Container, always_true_pred >();
            filtered_test_impl< Container, is_odd >();
            filtered_test_impl< Container, is_even >();
        }

        void filtered_test()
        {
            filtered_test_all_predicates< std::vector< int > >();
            filtered_test_all_predicates< std::list< int > >();
            filtered_test_all_predicates< std::set< int > >();
            filtered_test_all_predicates< std::multiset< int > >();
        }
    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.adaptor.filtered" );

    test->add( BOOST_TEST_CASE( &boost::filtered_test ) );

    return test;
}
