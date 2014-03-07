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
#include <boost/range/algorithm/mismatch.hpp>

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
        void eval_mismatch(Container1& cont1,
                           Container2& cont2,
                           BOOST_DEDUCED_TYPENAME range_iterator<Container1>::type ref_it1,
                           BOOST_DEDUCED_TYPENAME range_iterator<Container2>::type ref_it2
                           )
        {
            typedef BOOST_DEDUCED_TYPENAME range_iterator<Container1>::type iter1_t;
            typedef BOOST_DEDUCED_TYPENAME range_iterator<Container2>::type iter2_t;
            typedef std::pair<iter1_t, iter2_t> result_pair_t;
            
            result_pair_t result = boost::mismatch(cont1, cont2);
            BOOST_CHECK( result.first == ref_it1 );
            BOOST_CHECK( result.second == ref_it2 );
            
            result = boost::mismatch(boost::make_iterator_range(cont1),
                                     cont2);
            BOOST_CHECK( result.first == ref_it1 );
            BOOST_CHECK( result.second == ref_it2 );
            
            result = boost::mismatch(cont1,
                                     boost::make_iterator_range(cont2));
            BOOST_CHECK( result.first == ref_it1 );
            BOOST_CHECK( result.second == ref_it2 );
            
            result = boost::mismatch(boost::make_iterator_range(cont1),
                                     boost::make_iterator_range(cont2));
            BOOST_CHECK( result.first == ref_it1 );
            BOOST_CHECK( result.second == ref_it2 );
        }
        
        template< class Container1, class Container2, class Pred >
        void eval_mismatch(Container1& cont1,
                           Container2& cont2,
                           Pred pred,
                           BOOST_DEDUCED_TYPENAME range_iterator<Container1>::type ref_it1,
                           BOOST_DEDUCED_TYPENAME range_iterator<Container2>::type ref_it2
                           )
        {
            typedef BOOST_DEDUCED_TYPENAME range_iterator<Container1>::type iter1_t;
            typedef BOOST_DEDUCED_TYPENAME range_iterator<Container2>::type iter2_t;
            typedef std::pair<iter1_t, iter2_t> result_pair_t;
            
            result_pair_t result = boost::mismatch(cont1, cont2, pred);
            BOOST_CHECK( result.first == ref_it1 );
            BOOST_CHECK( result.second == ref_it2 );
            
            result = boost::mismatch(boost::make_iterator_range(cont1),
                                     cont2, pred);
            BOOST_CHECK( result.first == ref_it1 );
            BOOST_CHECK( result.second == ref_it2 );
            
            result = boost::mismatch(cont1,
                                     boost::make_iterator_range(cont2), pred);
            BOOST_CHECK( result.first == ref_it1 );
            BOOST_CHECK( result.second == ref_it2 );
            
            result = boost::mismatch(boost::make_iterator_range(cont1),
                                     boost::make_iterator_range(cont2),
                                     pred);
            BOOST_CHECK( result.first == ref_it1 );
            BOOST_CHECK( result.second == ref_it2 );
        }
        
        template< class Container1, class Container2 >
        void eval_mismatch(Container1& cont1,
                           Container2& cont2,
                           const int ref1,
                           const int ref2)
        {
            typedef BOOST_DEDUCED_TYPENAME range_iterator<Container1>::type iter1_t;
            typedef BOOST_DEDUCED_TYPENAME range_iterator<Container2>::type iter2_t;
            typedef std::pair<iter1_t, iter2_t> result_pair_t;
            
            result_pair_t result = boost::mismatch(cont1, cont2);
            BOOST_CHECK_EQUAL( ref1, *result.first );
            BOOST_CHECK_EQUAL( ref2, *result.second );
            
            result = boost::mismatch(boost::make_iterator_range(cont1), cont2);
            BOOST_CHECK_EQUAL( ref1, *result.first );
            BOOST_CHECK_EQUAL( ref2, *result.second );
            
            result = boost::mismatch(cont1, boost::make_iterator_range(cont2));
            BOOST_CHECK_EQUAL( ref1, *result.first );
            BOOST_CHECK_EQUAL( ref2, *result.second );
            
            result = boost::mismatch(boost::make_iterator_range(cont1),
                                     boost::make_iterator_range(cont2));
            BOOST_CHECK_EQUAL( ref1, *result.first );
            BOOST_CHECK_EQUAL( ref2, *result.second );
        }
        
        template< class Container1, class Container2, class Pred >
        void eval_mismatch(Container1& cont1,
                           Container2& cont2,
                           Pred pred,
                           const int ref1,
                           const int ref2)
        {
            typedef BOOST_DEDUCED_TYPENAME range_iterator<Container1>::type iter1_t;
            typedef BOOST_DEDUCED_TYPENAME range_iterator<Container2>::type iter2_t;
            typedef std::pair<iter1_t, iter2_t> result_pair_t;
            
            result_pair_t result = boost::mismatch(cont1, cont2, pred);
            BOOST_CHECK_EQUAL( ref1, *result.first );
            BOOST_CHECK_EQUAL( ref2, *result.second );
            
            result = boost::mismatch(boost::make_iterator_range(cont1),
                                     cont2, pred);
            BOOST_CHECK_EQUAL( ref1, *result.first );
            BOOST_CHECK_EQUAL( ref2, *result.second );
            
            result = boost::mismatch(cont1, boost::make_iterator_range(cont2),
                                     pred);
            BOOST_CHECK_EQUAL( ref1, *result.first );
            BOOST_CHECK_EQUAL( ref2, *result.second );
            
            result = boost::mismatch(boost::make_iterator_range(cont1),
                                     boost::make_iterator_range(cont2),
                                     pred);
            BOOST_CHECK_EQUAL( ref1, *result.first );
            BOOST_CHECK_EQUAL( ref2, *result.second );
        }
        
        template< class Container1, class Container2 >
        void test_mismatch_impl()
        {
            using namespace boost::assign;

            typedef BOOST_DEDUCED_TYPENAME boost::remove_const<Container1>::type MutableContainer1;
            typedef BOOST_DEDUCED_TYPENAME boost::remove_const<Container2>::type MutableContainer2;

            MutableContainer1 cont1;
            const Container1& cref_cont1 = cont1;
            MutableContainer2 cont2;
            const Container2& cref_cont2 = cont2;

            typedef BOOST_DEDUCED_TYPENAME Container1::iterator iterator1_t;
            typedef BOOST_DEDUCED_TYPENAME Container1::const_iterator const_iterator1_t;
            typedef BOOST_DEDUCED_TYPENAME Container2::iterator iterator2_t;
            typedef BOOST_DEDUCED_TYPENAME Container2::const_iterator const_iterator2_t;

            typedef std::pair<iterator1_t,       iterator2_t>       pair_mmit_t;
            typedef std::pair<const_iterator1_t, iterator2_t>       pair_cmit_t;
            typedef std::pair<iterator1_t,       const_iterator2_t> pair_mcit_t;
            typedef std::pair<const_iterator1_t, const_iterator2_t> pair_ccit_t;

            eval_mismatch(cont1, cont2, cont1.end(), cont2.end());
            eval_mismatch(cont1, cont2, std::equal_to<int>(), cont1.end(), cont2.end());
            eval_mismatch(cref_cont1, cont2, cref_cont1.end(), cont2.end());
            eval_mismatch(cref_cont1, cont2, std::equal_to<int>(), cref_cont1.end(), cont2.end());
            eval_mismatch(cont1, cref_cont2, cont1.end(), cref_cont2.end());
            eval_mismatch(cont1, cref_cont2, std::equal_to<int>(), cont1.end(), cref_cont2.end());
            eval_mismatch(cref_cont1, cref_cont2, cref_cont1.end(), cref_cont2.end());
            eval_mismatch(cref_cont1, cref_cont2, std::equal_to<int>(), cref_cont1.end(), cref_cont2.end());

            cont1 += 1,2,3,4;
            cont2 += 1,2,3,4;
            eval_mismatch(cont1, cont2, cont1.end(), cont2.end());
            eval_mismatch(cont1, cont2, std::equal_to<int>(), cont1.end(), cont2.end());
            eval_mismatch(cref_cont1, cont2, cref_cont1.end(), cont2.end());
            eval_mismatch(cref_cont1, cont2, std::equal_to<int>(), cref_cont1.end(), cont2.end());
            eval_mismatch(cont1, cref_cont2, cont1.end(), cref_cont2.end());
            eval_mismatch(cont1, cref_cont2, std::equal_to<int>(), cont1.end(), cref_cont2.end());
            eval_mismatch(cref_cont1, cref_cont2, cref_cont1.end(), cref_cont2.end());
            eval_mismatch(cref_cont1, cref_cont2, std::equal_to<int>(), cref_cont1.end(), cref_cont2.end());

            cont1.clear();
            cont2.clear();
            cont1 += 1,2,3,4;
            cont2 += 1,2,5,4;
            eval_mismatch(cont1, cont2, 3, 5);
            eval_mismatch(cont1, cont2, std::equal_to<int>(), 3, 5);
            eval_mismatch(cont1, cont2, std::not_equal_to<int>(), cont1.begin(), cont2.begin());
            eval_mismatch(cref_cont1, cont2, 3, 5);
            eval_mismatch(cref_cont1, cont2, std::equal_to<int>(), 3, 5);
            eval_mismatch(cref_cont1, cont2, std::not_equal_to<int>(), cref_cont1.begin(), cont2.begin());
            eval_mismatch(cont1, cref_cont2, 3, 5);
            eval_mismatch(cont1, cref_cont2, std::equal_to<int>(), 3, 5);
            eval_mismatch(cont1, cref_cont2, std::not_equal_to<int>(), cont1.begin(), cref_cont2.begin());
            eval_mismatch(cref_cont1, cref_cont2, 3, 5);
            eval_mismatch(cref_cont1, cref_cont2, std::equal_to<int>(), 3, 5);
            eval_mismatch(cref_cont1, cref_cont2, std::not_equal_to<int>(), cref_cont1.begin(), cref_cont2.begin());
        }

        void test_mismatch()
        {
            test_mismatch_impl< std::list<int>, std::list<int> >();
            test_mismatch_impl< const std::list<int>, std::list<int> >();
            test_mismatch_impl< std::list<int>, const std::list<int> >();
            test_mismatch_impl< const std::list<int>, const std::list<int> >();

            test_mismatch_impl< std::vector<int>, std::list<int> >();
            test_mismatch_impl< const std::vector<int>, std::list<int> >();
            test_mismatch_impl< std::vector<int>, const std::list<int> >();
            test_mismatch_impl< const std::vector<int>, const std::list<int> >();

            test_mismatch_impl< std::list<int>, std::vector<int> >();
            test_mismatch_impl< const std::list<int>, std::vector<int> >();
            test_mismatch_impl< std::list<int>, const std::vector<int> >();
            test_mismatch_impl< const std::list<int>, const std::vector<int> >();
        }
    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.algorithm.mismatch" );

    test->add( BOOST_TEST_CASE( &boost::test_mismatch ) );

    return test;
}
