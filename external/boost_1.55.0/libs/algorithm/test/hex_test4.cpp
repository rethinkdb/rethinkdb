/* 
   Copyright (c) Marshall Clow 2011-2012.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    For more information, see http://www.boost.org

Try ostream_iterators
*/

#include <boost/config.hpp>
#include <boost/algorithm/hex.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <string>
#include <iostream>

namespace ba = boost::algorithm;

void test_short_input1 () {
    std::string s;
    
    try { ba::unhex ( std::string ( "A" ), std::back_inserter(s)); }
    catch ( const std::exception &ex ) { return; }
    BOOST_TEST_MESSAGE ( "Failed to catch std::exception in test_short_input1" );
    BOOST_CHECK ( false );
    }

void test_short_input2 () {
    std::string s;
    
    try { ba::unhex ( std::string ( "A" ), std::back_inserter(s)); }
    catch ( const ba::hex_decode_error &ex ) { return; }
    BOOST_TEST_MESSAGE ( "Failed to catch ba::hex_decode_error in test_short_input2" );
    BOOST_CHECK ( false );
    }
    
void test_short_input3 () {
    std::string s;
    
    try { ba::unhex ( std::string ( "A" ), std::back_inserter(s)); }
    catch ( const ba::not_enough_input &ex ) { return; }
    BOOST_TEST_MESSAGE ( "Failed to catch ba::not_enough_input in test_short_input3" );
    BOOST_CHECK ( false );
    }
    
//  Make sure that the right thing is thrown
void test_short_input4 () {
    std::string s;
    
    try { ba::unhex ( std::string ( "A" ), std::back_inserter(s)); }
    catch ( const ba::non_hex_input &ex ) { BOOST_CHECK ( false ); }
    catch ( const ba::not_enough_input &ex ) { return; }
    catch ( ... ) { BOOST_CHECK ( false ); }
    BOOST_CHECK ( false );
    }

//  Make sure that the right thing is thrown
void test_short_input5 () {
    std::string s;
    
    try { ba::unhex ( "A", std::back_inserter(s)); }
    catch ( const ba::non_hex_input &ex ) { BOOST_CHECK ( false ); }
    catch ( const ba::not_enough_input &ex ) { return; }
    catch ( ... ) { BOOST_CHECK ( false ); }
    BOOST_CHECK ( false );
    }


void test_short_input () {
//  BOOST_TEST_MESSAGE ( "Short input tests for boost::algorithm::unhex" );
    test_short_input1 ();
    test_short_input2 ();
    test_short_input3 ();
    test_short_input4 ();
    test_short_input5 ();
    }


void test_nonhex_input1 () {
    std::string s;
    
    try { ba::unhex ( "01234FG1234", std::back_inserter(s)); }
    catch ( const std::exception &ex ) {
        BOOST_CHECK ( 'G' == *boost::get_error_info<ba::bad_char>(ex));
        return;
        }
    catch ( ... ) {}
    BOOST_TEST_MESSAGE ( "Failed to catch std::exception in test_nonhex_input1" );
    BOOST_CHECK ( false );
    }

void test_nonhex_input2 () {
    std::string s;
    
    try { ba::unhex ( "012Z4FA1234", std::back_inserter(s)); }
    catch ( const ba::hex_decode_error &ex ) {
        BOOST_CHECK ( 'Z' == *boost::get_error_info<ba::bad_char>(ex));
        return;
        }
    catch ( ... ) {}
    BOOST_TEST_MESSAGE ( "Failed to catch ba::hex_decode_error in test_nonhex_input2" );
    BOOST_CHECK ( false );
    }
    
void test_nonhex_input3 () {
    std::string s;
    
    try { ba::unhex ( "01234FA12Q4", std::back_inserter(s)); }
    catch ( const ba::non_hex_input &ex ) {
        BOOST_CHECK ( 'Q' == *boost::get_error_info<ba::bad_char>(ex));
        return;
        }
    catch ( ... ) {}
    BOOST_TEST_MESSAGE ( "Failed to catch ba::non_hex_input in test_nonhex_input3" );
    BOOST_CHECK ( false );
    }
    
//  Make sure that the right thing is thrown
void test_nonhex_input4 () {
    std::string s;
    
    try { ba::unhex ( "P1234FA1234", std::back_inserter(s)); }
    catch ( const ba::not_enough_input &ex ) { BOOST_CHECK ( false ); }
    catch ( const ba::non_hex_input &ex ) { return; }
    catch ( ... ) { BOOST_CHECK ( false ); }
    BOOST_CHECK ( false );
    }

void test_nonhex_input () {
//  BOOST_TEST_MESSAGE ( "Non hex input tests for boost::algorithm::unhex" );
    test_nonhex_input1 ();
    test_nonhex_input2 ();
    test_nonhex_input3 ();
    test_nonhex_input4 ();
    }

BOOST_AUTO_TEST_CASE( test_main )
{
    test_short_input ();
    test_nonhex_input ();
}
