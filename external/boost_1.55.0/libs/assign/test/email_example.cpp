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
#include <map>

namespace ba = boost::assign;

class email
{
public:
    enum address_option
    {
        check_addr_book,
        dont_check_addr_book
    };
    
    typedef std::pair<std::string,address_option> bcc_type;
    typedef std::vector< bcc_type >               bcc_map;
    typedef std::map<std::string,address_option>  address_map;
    
    
private:

    mutable address_map      cc_list;
    mutable address_map      to_list;
    bcc_map                  bcc_list;
            
    struct add_to_map
    {
        address_map& m;
    
        add_to_map( address_map& m ) : m(m)        
        {}
    
        void operator()( const std::string& name, address_option ao )
        {
            m[ name ] = ao; 
        }
        
        void operator()( const std::string& name )
        {
            m[ name ] = check_addr_book;
        }
    };

    struct add_to_vector
    {
        bcc_map& m;
        
        add_to_vector( bcc_map& m ) : m(m)
        {}
        
        void operator()( const bcc_type& r )
        {
            m.push_back( r );
        }
    };

public:
    
    ba::list_inserter< add_to_map >
    add_cc( std::string name, address_option ao )
    {
        return ba::make_list_inserter( add_to_map( cc_list ) )( name, ao );
    }

    ba::list_inserter< add_to_map >
    add_to( const std::string& name )
    {
        return ba::make_list_inserter( add_to_map( to_list ) )( name );
    }
    
    ba::list_inserter< add_to_vector, bcc_type >
    add_bcc( const bcc_type& bcc )
    {
        return ba::make_list_inserter( add_to_vector( bcc_list ) )( bcc );
    }
    
    address_option
    cc_at( const std::string& name ) const
    {
        return cc_list[ name ];
    }
    
    address_option 
    to_at( const std::string& name ) const
    {
        return to_list[ name ];
    }
    
    address_option
    bcc_at( unsigned index ) const
    {
        return bcc_list.at( index ).second;
    }
};



void check_list_inserter()
{
    using namespace boost::assign;

    email e;
    e.add_cc( "franz", email::dont_check_addr_book )
            ( "hanz", email::check_addr_book )
            ( "betty", email::dont_check_addr_book );
    BOOST_CHECK_EQUAL( e.cc_at( "franz" ), email::dont_check_addr_book );
    BOOST_CHECK_EQUAL( e.cc_at( "hanz" ), email::check_addr_book );
    BOOST_CHECK_EQUAL( e.cc_at( "betty" ), email::dont_check_addr_book );

    e.add_to( "betsy" )( "peter" );
    BOOST_CHECK_EQUAL( e.cc_at( "betsy" ), email::check_addr_book );
    BOOST_CHECK_EQUAL( e.cc_at( "peter" ), email::check_addr_book );
    
    e.add_bcc( email::bcc_type( "Mr. Foo", email::check_addr_book ) )
             ( "Mr. Bar", email::dont_check_addr_book );
    BOOST_CHECK_EQUAL( e.bcc_at( 0 ), email::check_addr_book );
    BOOST_CHECK_EQUAL( e.bcc_at( 1 ), email::dont_check_addr_book );
    
}


#include <boost/test/unit_test.hpp>
using boost::unit_test::test_suite;

test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite* test = BOOST_TEST_SUITE( "List Test Suite" );

    test->add( BOOST_TEST_CASE( &check_list_inserter ) );

    return test;
}

