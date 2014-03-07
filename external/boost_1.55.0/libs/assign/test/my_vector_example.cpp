// Boost.Assign library
//
//  Copyright Thorsten Ottosen 2003-2004. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/assign/
//


#include <boost/detail/workaround.hpp>

#if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564))
#  pragma warn -8091 // supress warning in Boost.Test
#  pragma warn -8057 // unused argument argc/argv in Boost.Test
#endif

#include <boost/assign/list_inserter.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <vector>
#include <stdexcept>

namespace ba = boost::assign;



template< class C >
class range_inserter
{
    typedef typename C::iterator iterator;
    iterator begin, end;
public:
    range_inserter( C& c )
    : begin( c.begin() ), end( c.end() )
    { }
    
    template< class T >
    void operator()( T r )
    {
        if( begin == end )
            throw std::range_error( "range error: <range_inserter>" );
        *begin = r;
        ++begin;
    }
};



template< class C >
inline range_inserter<C> make_range_inserter( C& c )
{
    return range_inserter<C>( c );
}



template< class T >
class my_vector
{
    typedef std::vector<T>                vector_t;
    typedef typename vector_t::size_type  size_type;
    vector_t data_;

public:
    my_vector() : data_( 10, 0 )
    { }
    
    ba::list_inserter< range_inserter< vector_t >, T >
    operator=( T r )
    {
        return ba::make_list_inserter( make_range_inserter( data_ ) )( r );
    }
    
    size_type size() const
    {
        return data_.size();
    }
    
    const T& operator[]( size_type index )
    {
        return data_.at( index );
    }
};



void check_list_inserter()
{
    using namespace std;
    using namespace boost::assign;
    
    my_vector<int> vec;
    vec = 1,2,3,4,5,6,7,8,9,10;
    BOOST_CHECK_EQUAL( vec.size(), 10u );
    BOOST_CHECK_EQUAL( vec[0], 1 );
    BOOST_CHECK_EQUAL( vec[9], 10 );
}



#include <boost/test/unit_test.hpp>
using boost::unit_test::test_suite;

test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite* test = BOOST_TEST_SUITE( "List Test Suite" );

    test->add( BOOST_TEST_CASE( &check_list_inserter ) );

    return test;
}

