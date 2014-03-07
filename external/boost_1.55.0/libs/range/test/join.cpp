// Boost.Range library
//
//  Copyright Neil Groves 2010. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#include <boost/range/join.hpp>

#include <boost/foreach.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/assign.hpp>
#include <boost/range/algorithm_ext.hpp>
#include <boost/range/irange.hpp>

#include <boost/iterator/iterator_facade.hpp>

#include <algorithm>
#include <deque>
#include <list>
#include <vector>

namespace boost
{
    namespace
    {
        // This function is a helper function that writes integers
        // of increasing value into a range. It is used to test
        // that joined ranged may be written to.
        //
        // Requires:
        // - Range uses shallow copy semantics.
        template< typename Range >
        void fill_with_ints(Range rng)
        {
            typedef typename range_iterator<Range>::type iterator;
            iterator target = boost::begin(rng);
            const int count = boost::distance(rng);
            for (int i = 0; i < count; ++i)
            {
                *target = i;
                ++target;
            }
        }

        // The test_join_traversal function is used to provide additional
        // tests based upon the underlying join iterator traversal.
        // The join iterator takes care of the appropriate demotion, and
        // this demotion.

        // test_join_traversal - additional tests for input and forward
        // traversal iterators. This is of course a no-op.
        template< typename Range1, typename Range2, typename TraversalTag >
        void test_join_traversal(Range1& rng1, Range2& rng2, TraversalTag)
        {
        }

        // test_join_traversal - additional tests for bidirectional
        // traversal iterators.
        template< typename Range1, typename Range2 >
        void test_join_traversal(Range1& rng1, Range2& rng2, boost::bidirectional_traversal_tag)
        {
            typedef typename range_value<Range1>::type value_type;
            std::vector<value_type> reference(boost::begin(rng1), boost::end(rng1));
            boost::push_back(reference, rng2);
            std::reverse(reference.begin(), reference.end());

            std::vector<value_type> test_result;
            BOOST_REVERSE_FOREACH( value_type x, join(rng1, rng2) )
            {
                test_result.push_back(x);
            }

            BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                                           test_result.begin(), test_result.end() );
        }

        // Test helper function to implement the additional tests for random
        // access traversal iterators. This is used by the test_join_traversal
        // function for random access iterators. The reason that the test
        // implementation is put into this function is to utilise
        // template parameter type deduction for the joined range type.
        template< typename Range1, typename Range2, typename JoinedRange >
        void test_random_access_join(Range1& rng1, Range2& rng2, JoinedRange joined)
        {
            BOOST_CHECK_EQUAL( boost::end(joined) - boost::begin(joined), boost::distance(joined) );
            BOOST_CHECK( boost::end(joined) <= boost::begin(joined) );
            BOOST_CHECK( boost::begin(joined) >= boost::end(joined) );
            if (boost::empty(joined))
            {
                BOOST_CHECK(!(boost::begin(joined) < boost::end(joined)));
                BOOST_CHECK(!(boost::end(joined) > boost::begin(joined)));
            }
            else
            {
                BOOST_CHECK(boost::begin(joined) < boost::end(joined));
                BOOST_CHECK(boost::end(joined) < boost::begin(joined));
            }

            typedef typename boost::range_difference<JoinedRange>::type difference_t;
            const difference_t count = boost::distance(joined);
            BOOST_CHECK( boost::begin(joined) + count == boost::end(joined) );
            BOOST_CHECK( boost::end(joined) - count == boost::begin(joined) );

            typedef typename boost::range_iterator<JoinedRange>::type iterator_t;
            iterator_t it = boost::begin(joined);
            it += count;
            BOOST_CHECK( it == boost::end(joined) );

            it = boost::end(joined);
            it -= count;
            BOOST_CHECK( it == boost::begin(joined) );
        }

        // test_join_traversal function for random access traversal joined
        // ranges.
        template< typename Range1, typename Range2 >
        void test_join_traversal(Range1& rng1, Range2& rng2, boost::random_access_traversal_tag)
        {
            test_join_traversal(rng1, rng2, boost::bidirectional_traversal_tag());
            test_random_access_join(rng1, rng2, join(rng1, rng2));
        }

        // Test the ability to write values into a joined range. This is
        // achieved by copying the constant collections, altering them
        // and then checking the result. Hence this relies upon both
        // rng1 and rng2 having value copy semantics.
        template< typename Collection1, typename Collection2 >
        void test_write_to_joined_range(const Collection1& rng1, const Collection2& rng2)
        {
            Collection1 c1(rng1);
            Collection2 c2(rng2);
            typedef typename boost::range_value<Collection1>::type value_t;
            fill_with_ints(boost::join(c1,c2));

            // Ensure that the size of the written range has not been
            // altered.
            BOOST_CHECK_EQUAL( boost::distance(c1), boost::distance(rng1) );
            BOOST_CHECK_EQUAL( boost::distance(c2), boost::distance(rng2) );

            // For each element x, in c1 ensure that it has been written to
            // with incrementing integers
            int x = 0;
            typedef typename range_iterator<Collection1>::type iterator1;
            iterator1 it1 = boost::begin(c1);
            for (; it1 != boost::end(c1); ++it1)
            {
                BOOST_CHECK_EQUAL( x, *it1 );
                ++x;
            }

            // For each element y, in c2 ensure that it has been written to
            // with incrementing integers
            typedef typename range_iterator<Collection2>::type iterator2;
            iterator2 it2 = boost::begin(c2);
            for (; it2 != boost::end(c2); ++it2)
            {
                BOOST_CHECK_EQUAL( x, *it2 );
                ++x;
            }
        }

        // Perform a unit test of a Boost.Range join() comparing
        // it to a reference that is populated by appending
        // elements from both source ranges into a vector.
        template< typename Collection1, typename Collection2 >
        void test_join_impl(Collection1& rng1, Collection2& rng2)
        {
            typedef typename range_value<Collection1>::type value_type;
            std::vector<value_type> reference(boost::begin(rng1), boost::end(rng1));
            boost::push_back(reference, rng2);

            std::vector<value_type> test_result;
            boost::push_back(test_result, join(rng1, rng2));

            BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                                           test_result.begin(), test_result.end() );

            typedef boost::range_detail::join_iterator<
                typename boost::range_iterator<Collection1>::type,
                typename boost::range_iterator<Collection2>::type
                > join_iterator_t;

            typedef boost::iterator_traversal< join_iterator_t > tag_t;

           test_join_traversal(rng1, rng2, tag_t());

           test_write_to_joined_range(rng1, rng2);
        }

        // Make a collection filling it with items from the source
        // range. This is used to build collections of various
        // sizes populated with various values designed to optimize
        // the code coverage exercised by the core test function
        // test_join_impl.
        template<typename Collection, typename Range>
        boost::shared_ptr<Collection> makeCollection(const Range& source)
        {
            boost::shared_ptr<Collection> c(new Collection);
            c->insert(c->end(), boost::begin(source), boost::end(source));
            return c;
        }

        // This templatised version of the test_join_impl function
        // generates and populates collections which are later
        // used as input to the core test function.
        // The caller of this function explicitly provides the
        // template parameters. This supports the generation
        // of testing a large combination of range types to be
        // joined. It is of particular importance to remember
        // to combine a random_access range with a bidirectional
        // range to determine that the correct demotion of
        // types occurs in the join_iterator.
        template< typename Collection1, typename Collection2 >
        void test_join_impl()
        {
            typedef boost::shared_ptr<Collection1> collection1_ptr;
            typedef boost::shared_ptr<Collection2> collection2_ptr;
            typedef boost::shared_ptr<const Collection1> collection1_cptr;
            typedef boost::shared_ptr<const Collection2> collection2_cptr;
            std::vector< collection1_cptr > left_containers;
            std::vector< collection2_cptr > right_containers;

            left_containers.push_back(collection1_ptr(new Collection1));
            left_containers.push_back(makeCollection<Collection1>(irange(0,1)));
            left_containers.push_back(makeCollection<Collection1>(irange(0,100)));

            right_containers.push_back(collection2_ptr(new Collection2));
            right_containers.push_back(makeCollection<Collection2>(irange(0,1)));
            right_containers.push_back(makeCollection<Collection2>(irange(0,100)));

            BOOST_FOREACH( collection1_cptr left_container, left_containers )
            {
                BOOST_FOREACH( collection2_cptr right_container, right_containers )
                {
                    test_join_impl(*left_container, *right_container);
                }
            }
        }

        // entry-point into the unit test for the join() function
        // this tests a representative sample of combinations of
        // source range type.
        void join_test()
        {
            test_join_impl< std::vector<int>, std::vector<int> >();
            test_join_impl< std::list<int>,   std::list<int>   >();
            test_join_impl< std::deque<int>,  std::deque<int>  >();

            test_join_impl< std::vector<int>, std::list<int>   >();
            test_join_impl< std::list<int>,   std::vector<int> >();
            test_join_impl< std::vector<int>, std::deque<int>  >();
            test_join_impl< std::deque<int>,  std::vector<int> >();
        }

    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.adaptor.joined" );

    test->add( BOOST_TEST_CASE( &boost::join_test ) );

    return test;
}
