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


#include <boost/assign/ptr_map_inserter.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <typeinfo>
#include <string>

//
// abstract base class definition
// 
struct abstract_base
{
    virtual ~abstract_base() {}
    virtual void foo() = 0;
    virtual abstract_base* clone() const = 0;
};

struct implementation : abstract_base
{
    implementation()
    { }

    implementation( const implementation& )
    { }

    implementation( int )
    { }

    implementation( int, int )
    { }
 
    implementation( int, std::string, int, std::string )
    { }

    virtual void foo() {}
    virtual abstract_base* clone() const
    {
        return new implementation( *this );
    }
};


void check_ptr_map_inserter()
{
    
#ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING

    boost::ptr_map<std::string, abstract_base> m;
    boost::assign::ptr_map_insert<implementation>( m )
                                       ( "foo", 1, "two", 3, "four" )
                                       ( "bar", 41, "42", 43, "44" );
    BOOST_CHECK_EQUAL( m.size(), 2u );
    BOOST_CHECK( typeid(m.at("foo")) == typeid(implementation) );

#endif

    boost::ptr_map<std::string,implementation> m2;
    boost::assign::ptr_map_insert( m2 )
                     ( "foobar", 1, "two", 3, "four" )
                     ( "key1" )( "key2" )( "key3" )( "key4" )
                     ( "key5", 42 )( "key6", 42, 42 );
    
    BOOST_CHECK_EQUAL( m2.size(), 7u );
    boost::assign::ptr_map_insert( m2 )( "key1" ); 
    BOOST_CHECK_EQUAL( m2.size(), 7u ); // duplicates not inserted   
    
}



#include <boost/test/unit_test.hpp>
using boost::unit_test::test_suite;

test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite* test = BOOST_TEST_SUITE( "List Test Suite" );

    test->add( BOOST_TEST_CASE( &check_ptr_map_inserter ) );

    return test;
}

