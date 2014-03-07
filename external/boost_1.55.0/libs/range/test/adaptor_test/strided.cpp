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
// The strided_defect_Trac5014 test case is a modified version of a test case
// contributed by Michel Morin as part of the trac ticket.
//
#include <boost/range/adaptor/strided.hpp>

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/assign.hpp>
#include <boost/range/algorithm_ext.hpp>

#include <algorithm>
#include <deque>
#include <vector>

namespace boost
{
    namespace
    {
        template< class Container >
        void strided_test_impl( Container& c, int stride_size )
        {
            using namespace boost::adaptors;

            // Rationale:
            // This requirement was too restrictive. It makes the use of the
            // strided adaptor too dangerous, and a simple solution existed
            // to make it safe, hence the strided adaptor has been modified
            // and this restriction no longer applies.
            //BOOST_ASSERT( c.size() % STRIDE_SIZE == 0 );

            Container reference;

            {
                typedef BOOST_DEDUCED_TYPENAME Container::const_iterator iterator_t;
                typedef BOOST_DEDUCED_TYPENAME Container::difference_type diff_t;
                typedef BOOST_DEDUCED_TYPENAME Container::size_type size_type;
                iterator_t it = c.begin();

                iterator_t last = c.end();
                for (; it != last; )
                {
                    reference.push_back(*it);

                    for (int i = 0; (it != last) && (i < stride_size); ++i)
                        ++it;
                }
            }

            Container test;
            boost::push_back( test, c | strided(stride_size) );

            BOOST_CHECK_EQUAL_COLLECTIONS( test.begin(), test.end(),
                reference.begin(), reference.end() );

            Container test2;
            boost::push_back( test2, adaptors::stride(c, stride_size) );

            BOOST_CHECK_EQUAL_COLLECTIONS( test2.begin(), test2.end(),
                reference.begin(), reference.end() );

            // Test the const versions:
            const Container& cc = c;
            Container test3;
            boost::push_back( test3, cc | strided(stride_size) );

            BOOST_CHECK_EQUAL_COLLECTIONS( test3.begin(), test3.end(),
                reference.begin(), reference.end() );

            Container test4;
            boost::push_back( test4, adaptors::stride(cc, stride_size) );

            BOOST_CHECK_EQUAL_COLLECTIONS( test4.begin(), test4.end(),
                reference.begin(), reference.end() );
        }

        template< class Container >
        void strided_test_impl(int stride_size)
        {
            using namespace boost::assign;

            Container c;

            // Test empty
            strided_test_impl(c, stride_size);

            // Test two elements
            c += 1,2;
            strided_test_impl(c, stride_size);

            // Test many elements
            c += 1,1,1,2,2,3,4,5,6,6,6,7,8,9;
            strided_test_impl(c, stride_size);

            // Test an odd number of elements to determine that the relaxation
            // of the requirements has been successful
            // Test a sequence of length 1 with a stride of 2
            c.clear();
            c += 1;
            strided_test_impl(c, stride_size);

            // Test a sequence of length 2 with a stride of 2
            c.clear();
            c += 1,2;
            strided_test_impl(c, stride_size);

            // Test a sequence of length 3 with a stride of 2
            c.clear();
            c += 1,2,3;
            strided_test_impl(c, stride_size);
        }

        template<typename Container>
        void strided_test_zero_stride()
        {
            Container c;
            c.push_back(1);

            typedef boost::strided_range<Container> strided_range_t;
            strided_range_t rng( boost::adaptors::stride(c, 0) );
            typedef BOOST_DEDUCED_TYPENAME boost::range_iterator<strided_range_t>::type iter_t;

            iter_t first(boost::begin(c), boost::begin(c), boost::end(c), 0);
            iter_t last(boost::begin(c), boost::end(c), boost::end(c), 0);

            iter_t it = first;
            for (int i = 0; i < 10; ++i, ++it)
            {
                BOOST_CHECK(it == first);
            }
        }

        template<typename Container>
        void strided_test_impl()
        {
            strided_test_zero_stride< Container >();

            const int MAX_STRIDE_SIZE = 10;
            for (int stride_size = 1; stride_size <= MAX_STRIDE_SIZE; ++stride_size)
            {
                strided_test_impl< Container >(stride_size);
            }
        }

        void strided_test()
        {
            strided_test_impl< std::vector<int> >();
            strided_test_impl< std::deque<int> >();
            strided_test_impl< std::list<int> >();
        }

        void strided_defect_Trac5014()
        {
            using namespace boost::assign;

            std::vector<int> v;
            for (int i = 0; i < 30; ++i)
                v.push_back(i);

            std::vector<int> reference;
            reference += 0,4,8,12,16,20,24,28;

            std::vector<int> output;
            boost::push_back(output, v | boost::adaptors::strided(4));

            BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                                           output.begin(), output.end() );

            BOOST_CHECK_EQUAL( output.back(), 28 );
        }

        template<typename BaseIterator, typename Category>
        class strided_mock_iterator
            : public boost::iterator_adaptor<
                strided_mock_iterator<BaseIterator,Category>
              , BaseIterator
              , boost::use_default
              , Category
            >
        {
            typedef boost::iterator_adaptor<
                        strided_mock_iterator
                      , BaseIterator
                      , boost::use_default
                      , Category
                    > super_t;
        public:
            explicit strided_mock_iterator(BaseIterator it)
                : super_t(it)
            {
            }

        private:
            void increment()
            {
                ++(this->base_reference());
            }

            friend class boost::iterator_core_access;
        };

        template<typename Category, typename Range>
        boost::iterator_range<strided_mock_iterator<BOOST_DEDUCED_TYPENAME boost::range_iterator<Range>::type, Category> >
        as_mock_range(Range& rng)
        {
            typedef BOOST_DEDUCED_TYPENAME boost::range_iterator<Range>::type range_iter_t;
            typedef strided_mock_iterator<range_iter_t, Category> mock_iter_t;

            return boost::iterator_range<mock_iter_t>(
                      mock_iter_t(boost::begin(rng)),
                      mock_iter_t(boost::end(rng)));
        }

        void strided_test_traversal()
        {
            using namespace boost::assign;

            std::vector<int> v;
            for (int i = 0; i < 30; ++i)
                v.push_back(i);

            std::vector<int> reference;
            reference += 0,4,8,12,16,20,24,28;

            std::vector<int> output;
            boost::push_back(output, as_mock_range<boost::forward_traversal_tag>(v) | boost::adaptors::strided(4));

            BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                                           output.begin(), output.end() );

            output.clear();
            boost::push_back(output, as_mock_range<boost::bidirectional_traversal_tag>(v) | boost::adaptors::strided(4));

            BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                                           output.begin(), output.end() );

            output.clear();
            boost::push_back(output, as_mock_range<boost::random_access_traversal_tag>(v) | boost::adaptors::strided(4));

            BOOST_CHECK_EQUAL_COLLECTIONS( reference.begin(), reference.end(),
                                           output.begin(), output.end() );
        }

        template<typename Range>
        void strided_test_ticket_5236_check_bidirectional(const Range& rng)
        {
            BOOST_CHECK_EQUAL( boost::distance(rng), 1 );
            BOOST_CHECK_EQUAL( std::distance(boost::begin(rng), boost::prior(boost::end(rng))), 0 );
        }
        
        template<typename Range>
        void strided_test_ticket_5236_check(const Range& rng)
        {
            strided_test_ticket_5236_check_bidirectional(rng);
                        
            typename boost::range_iterator<const Range>::type it = boost::end(rng);
            it = it - 1;
            BOOST_CHECK_EQUAL( std::distance(boost::begin(rng), it), 0 );
        }
        
        void strided_test_ticket_5236()
        {
            std::vector<int> v;
            v.push_back(1);
            strided_test_ticket_5236_check( v | boost::adaptors::strided(2) );                
            
            // Ensure that there is consistency between the random-access implementation
            // and the bidirectional.
            
            std::list<int> l;
            l.push_back(1);
            strided_test_ticket_5236_check_bidirectional( l | boost::adaptors::strided(2) );
        }
        
    }
}

boost::unit_test::test_suite*
init_unit_test_suite(int argc, char* argv[])
{
    boost::unit_test::test_suite* test
        = BOOST_TEST_SUITE( "RangeTestSuite.adaptor.strided" );

    test->add( BOOST_TEST_CASE( &boost::strided_test ) );
    test->add( BOOST_TEST_CASE( &boost::strided_defect_Trac5014 ) );
    test->add( BOOST_TEST_CASE( &boost::strided_test_traversal ) );
    test->add( BOOST_TEST_CASE( &boost::strided_test_ticket_5236 ) );

    return test;
}
