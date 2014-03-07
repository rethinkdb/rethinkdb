//  (C) Copyright Gennadiy Rozental 2002-2008.
//  (C) Copyright Gennadiy Rozental & Ullrich Koethe 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

// Boost.Test
#include <boost/test/unit_test.hpp>
#include <boost/test/parameterized_test.hpp>
using namespace boost::unit_test;

// STL
#include <vector>
#include <string>

#define LOG_FATAL_ERROR( M )                                  \
BOOST_TEST_LOG_ENTRY( ::boost::unit_test::log_fatal_errors )  \
    << (::boost::unit_test::lazy_ostream::instance() << M)
//____________________________________________________________________________//

// this free function is invoked with all parameters specified in a list
void check_string( std::string const& s )
{
    // reports 'error in "check_string": test s.substr( 0, 3 ) == "hdr" failed [actual_value != hdr]'
    BOOST_CHECK_EQUAL( s.substr( 0, 3 ), "hdr" );
}

//____________________________________________________________________________//

test_suite*
init_unit_test_suite( int /*argc*/, char* /*argv*/[] ) {
    framework::master_test_suite().p_name.value = "Unit test example 11";

    LOG_FATAL_ERROR( "something happend" );

    // parameters have no requirements to stay alive beyond the next statement
    std::string const params[] = { "hdr1 ", "hdr2", "3  " };

    framework::master_test_suite().add( 
        BOOST_PARAM_TEST_CASE( &check_string, (std::string const*)params, params+3 ) );

    return 0; 
}

//____________________________________________________________________________//

// EOF
