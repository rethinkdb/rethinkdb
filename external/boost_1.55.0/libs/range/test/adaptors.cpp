// Boost.Range library
//
//  Copyright Thorsten Ottosen 2006. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//

#include <boost/range/adaptors.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>
#include <boost/assign/list_of.hpp>
#include <vector>
#include <list>
#include <string>
#include <map>

template< class T >
struct less_than
{
    T val;

    less_than() : val(0)
    {}
    
    less_than( T t ) : val(t)
    {}

    bool operator()( const T& r ) const
    {
        return r < val;
    }
};



template< class T >
struct multiply
{
    T val;
    
    typedef T& result_type;

    multiply( T t ) : val(t)
    { }
    
    T& operator()( T& r ) const
    {
        return r *= 2;
    }
};



template< class Rng >
void check_copy( Rng r )
{
    //
    // Make sure the generated iterators
    // can actually be copied
    //
    Rng r2 = r;
    r2     = r;
}


template< class Rng >
void check_direct()
{
    using namespace boost::adaptors;
    
    Rng rng = boost::assign::list_of(1)(2)(3)(4)(5).to_container( rng );
    Rng out;

    //
    // test each alphabetically
    //
    BOOST_FOREACH( int i, rng | filtered( less_than<int>(4) ) 
                              /*| reversed*/ 
                              | transformed( multiply<int>(2) ) )
    {
      out.push_back( i );
    }

    BOOST_CHECK_EQUAL( out.size(), 3u );
    BOOST_CHECK_EQUAL( *out.begin(), 2 );
    BOOST_CHECK_EQUAL( *boost::next(out.begin()), 4 );
    BOOST_CHECK_EQUAL( *boost::next(out.begin(),2), 6 );

    rng = boost::assign::list_of(1)(1)(2)(2)(3)(3)(4)(5).to_container( rng );
    out.clear();
    /*
    BOOST_FOREACH( int i, rng | adjacent_filtered( std::equal_to<int>() )
                              | uniqued )
    {
        
        out.push_back( i );
    }*/
    
}


template< class IndirectRng >
void check_indirect()
{
    using namespace boost::adaptors;

    IndirectRng rng;

    std::vector< boost::shared_ptr< int > > holder;
    
    for( unsigned i = 0u; i != 20u; ++i )
    {
        boost::shared_ptr<int> v(new int(i));
        rng.push_back( v.get() );
    }

    BOOST_FOREACH( int& i, rng | indirected | reversed 
                               | transformed( multiply<int>(2) ) )
    {
        i += 1;
    }

    
    
}



template< class RandomAccessRng >
void check_random_access()
{
    using namespace boost::adaptors;
    
    RandomAccessRng rng(1, 20u);
    RandomAccessRng out;

    BOOST_FOREACH( int i, rng | reversed 
                              | transformed( multiply<int>(2) )  
                              /* | sliced(0,15) */ )
    {
      out.push_back( i );
    }

    
    BOOST_FOREACH( int i, rng | copied(3u,13u) )
    {
        out.push_back( i );
    }     
}



template< class Map >
void check_map()
{
    using namespace boost::adaptors;
    
    Map m;
    m.insert( std::make_pair(1,2) );
    m.insert( std::make_pair(2,2) );
    m.insert( std::make_pair(3,2) );
    m.insert( std::make_pair(4,2) );
    m.insert( std::make_pair(5,2) );
    m.insert( std::make_pair(6,2) );
    m.insert( std::make_pair(7,2) );

    std::vector<int> keys 
        = boost::copy_range< std::vector<int> >( m | map_keys );
    std::vector<int> values 
       = boost::copy_range< std::vector<int> >( m | map_values );
}



void check_regex()
{
    using namespace boost::adaptors;
    std::string s("This is a string of tokens");
    std::vector<std::string> tokens =
        boost::copy_range< std::vector<std::string> >( s | tokenized( "\\s+", -1 ) );
}


void check_adaptors()
{
    check_direct< std::vector<int> >();
    check_direct< std::list<int> >();
    check_indirect< std::vector<int*> >();
    check_indirect< std::list<int*> >();

    check_map< std::map<int,int> >();
//    check_random_access< std::vector<int> >();
    check_regex();

    using namespace boost::adaptors;
    std::vector<int>  vec(10u,20);
    std::vector<int*> pvec;
    std::map<int,int> map;
    
    check_copy( vec | adjacent_filtered( std::equal_to<int>() ) );
  //  check_copy( vec | indexed );
    check_copy( vec | reversed );
    check_copy( vec | uniqued );
    check_copy( pvec | indirected );

//    check_copy( vec | sliced(1,5) );
    //
    // This does not return an iterator_range<>, so
    // won't pass this test of implicit conversion
    //  check_copy( vec | copied(1,5) );
    //
    check_copy( map | map_values );
    check_copy( map | map_keys );
    check_copy( std::string( "a string" ) | tokenized( "\\s+", -1 ) );
    check_copy( vec | filtered( less_than<int>(2) ) );
    check_copy( vec | transformed( multiply<int>(2) ) ); 
}

using boost::unit_test::test_suite;

test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    using namespace boost;

    test_suite* test = BOOST_TEST_SUITE( "Range Test Suite - Adaptors" );

    test->add( BOOST_TEST_CASE( &check_adaptors ) );

    return test;
}


