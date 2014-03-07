//
// Boost.Pointer Container
//
//  Copyright Thorsten Ottosen 2008. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/ptr_container/
//

#include <boost/ptr_container/ptr_inserter.hpp> 
#include <boost/ptr_container/indirect_fun.hpp>
#include <boost/ptr_container/ptr_deque.hpp>
#include <boost/ptr_container/ptr_list.hpp>
#include <boost/assign/list_inserter.hpp> 
#include <boost/iterator/transform_iterator.hpp>
#include <boost/test/test_tools.hpp>
#include <algorithm>
#include <functional>
#include <string>

template< class T >
struct caster_to
{
    typedef T result_type;
    
    T operator()( void* obj ) const
    {
        return static_cast<T>( obj );
    }
};

template< class PtrSequence >
void test_ptr_inserter_helper()
{
    using namespace boost;
    PtrSequence seq;
    const int size = 1000;
    for( int i = 0; i != size; ++i )
        seq.push_back( i % 3 == 0 ? 0 : new int(i) );

    PtrSequence seq2;
    //
    // @remark: we call .base() to avoid null pointer indirection.
    //          The clone_inserter will handle the nulls correctly. 
    //
    std::copy( boost::make_transform_iterator( seq.begin().base(), caster_to<int*>() ),
               boost::make_transform_iterator( seq.end().base(), caster_to<int*>() ),
               ptr_container::ptr_back_inserter( seq2 ) );

    std::copy( boost::make_transform_iterator( seq.begin().base(), caster_to<int*>() ),
               boost::make_transform_iterator( seq.end().base(), caster_to<int*>() ),
               ptr_container::ptr_front_inserter( seq2 ) );
    BOOST_CHECK_EQUAL( seq.size()*2, seq2.size() );

    PtrSequence seq3;
    for( int i = 0; i != size; ++i )
        seq3.push_back( new int(i%3) );

    //
    // @remark: since there are no nulls in this container, it
    //          is easier to handle.
    //
    std::copy( seq3.begin(), seq3.end(), 
               ptr_container::ptr_inserter( seq, seq.end() ) ); 
    BOOST_CHECK_EQUAL( seq.size(), seq2.size() );
}


void test_ptr_inserter()
{
    test_ptr_inserter_helper< boost::ptr_list< boost::nullable<int> > >();
    test_ptr_inserter_helper< boost::ptr_deque< boost::nullable<int> > >();


}



#include <boost/test/unit_test.hpp>
using boost::unit_test::test_suite;

test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite* test = BOOST_TEST_SUITE( "Pointer Container Test Suite" );

    test->add( BOOST_TEST_CASE( &test_ptr_inserter ) );

    return test;
}


