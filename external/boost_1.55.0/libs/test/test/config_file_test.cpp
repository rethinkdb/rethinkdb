//  (C) Copyright Gennadiy Rozental 2001-2008.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision: 49313 $
//
//  Description : basic_cstring unit test
// *****************************************************************************

// Boost.Test
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
namespace utf = boost::unit_test;

// Boost.Runtime.Parameter
#include <boost/test/utils/runtime/file/config_file.hpp>
#include <boost/test/utils/runtime/validation.hpp>

namespace rt  = boost::runtime;
namespace rtf = boost::runtime::file;

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( basic_load_test ) 
{
    rtf::config_file cf( "test_files/cfg_file_tst1.cfg" );

    BOOST_CHECK_EQUAL( rtf::get_requ_param_value( cf, "par1" ), "ABC " );

    BOOST_CHECK_EQUAL( rtf::get_requ_param_value( cf, "NS1", "par1" ), "12" );
    BOOST_CHECK_EQUAL( rtf::get_requ_param_value( cf, "NS2", "NS3", "par1" ), "OFF" );
    BOOST_CHECK_EQUAL( rtf::get_requ_param_value( cf, "NS2", "NS4", "par1" ), "ON" );
    BOOST_CHECK_EQUAL( rtf::get_requ_param_value( cf, "NS2", "NS4", "NS5", "par1" ), "1 2 3" );

    BOOST_CHECK( !rtf::get_param_value( cf, "par1 " ) );
    BOOST_CHECK( !rtf::get_param_value( cf, "par2" ) );
    BOOST_CHECK( !rtf::get_param_value( cf, "NS2", "par1" ) );

    BOOST_CHECK_THROW( rtf::get_requ_param_value( cf, "par1 " ), rt::logic_error );
    BOOST_CHECK_THROW( rtf::get_requ_param_value( cf, "par2" ), rt::logic_error );
    BOOST_CHECK_THROW( rtf::get_requ_param_value( cf, "NS2", "par1" ), rt::logic_error );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( multiple_load ) 
{
    rtf::config_file cf;

    cf.load( "test_files/cfg_file_tst3.cfg" );
    cf.load( "test_files/cfg_file_tst4.cfg" );

    BOOST_CHECK_EQUAL( rtf::get_requ_param_value( cf, "par1" ), "1" );
    BOOST_CHECK_EQUAL( rtf::get_requ_param_value( cf, "NS", "par2" ), "1 2" );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( custom_value_marker ) 
{
    rtf::config_file cf;
        
    cf.load( "test_files/cfg_file_tst2.cfg", rtf::value_marker = "|" );

    BOOST_CHECK_EQUAL( rtf::get_requ_param_value( cf, "par1" ), "\"Simple text \"" );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( custom_value_delimeter ) 
{
    rtf::config_file cf;

    cf.load( "test_files/cfg_file_tst5.cfg", rtf::value_delimeter = "=> " );
    cf.load( "test_files/cfg_file_tst6.cfg", rtf::value_delimeter = " " );

    BOOST_CHECK_EQUAL( rtf::get_requ_param_value( cf, "par1" ), "1" );
    BOOST_CHECK_EQUAL( rtf::get_requ_param_value( cf, "NS", "par2" ), "2" );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( custom_ns_delimeter ) 
{
    rtf::config_file cf;

    cf.load( "test_files/cfg_file_tst7.cfg", (rtf::namespace_delimeter = "/",rtf::value_delimeter = " ") );

    BOOST_CHECK_EQUAL( rtf::get_requ_param_value( cf, "NS1", "NS2", "par" ), "1" );
}

//____________________________________________________________________________//

#if 0

void test_aliases()
{
    global_param_namespace.clear();

    config_file_iterator cfi1( "test_files/par_hdl_tst1.cfg" );
    BOOST_CHECK( global_param_namespace.load_parameters( cfi1 ) );

    config_file_iterator cfi2( "test_files/par_alias1.cfg" );
    BOOST_CHECK( global_param_namespace.load_aliases( cfi2 ) );

    BOOST_CHECK_EQUAL( rtf::get_param_value( "par1" ), "ABC" );
    BOOST_CHECK_EQUAL( rtf::get_param_value( "par2" ), "12" );
    BOOST_CHECK_EQUAL( rtf::get_param_value( "par3" ), "OFF" );
    BOOST_CHECK_EQUAL( rtf::get_param_value( "par4" ), "ON" );
    BOOST_CHECK_EQUAL( rtf::get_param_value( "par5" ), "OFF" );
}

//____________________________________________________________________________//

void test_validations()
{
    BOOST_CHECK_THROW( global_param_namespace.insert_param( "PAR-AM" ), fnd_runtime_exception );
    BOOST_CHECK_THROW( global_param_namespace.insert_param( "p^" ), fnd_runtime_exception );
    BOOST_CHECK_THROW( global_param_namespace.insert_namespace( "ns#1" ), fnd_runtime_exception );
    BOOST_CHECK_THROW( global_param_namespace.insert_namespace( "ns 1" ), fnd_runtime_exception );
    BOOST_CHECK_THROW( global_param_namespace.insert_namespace( "ns-1" ), fnd_runtime_exception );

    config_file_iterator cfi1( "test_files/par_hdl_tst2.cfg" );
    BOOST_CHECK( !global_param_namespace.load_parameters( cfi1 ) );

    config_file_iterator cfi2( "test_files/par_alias2.cfg" );
    BOOST_CHECK( !global_param_namespace.load_aliases( cfi2 ) );

    config_file_iterator cfi3( "test_files/par_alias3.cfg" );
    BOOST_CHECK( !global_param_namespace.load_aliases( cfi3 ) );
}

//____________________________________________________________________________//

#define QUOTE_N_END( string ) "\"" string "\"\n"
void test_io()
{
    global_param_namespace.clear();

    config_file_iterator cfi1( "test_files/par_hdl_tst1.cfg" );
    BOOST_CHECK( global_param_namespace.load_parameters( cfi1 ) );

    config_file_iterator cfi2( "test_files/par_alias1.cfg" );
    BOOST_CHECK( global_param_namespace.load_aliases( cfi2 ) );

    output_test_stream ots;

    ots << global_param_namespace;

    BOOST_CHECK( ots.is_equal( 
        "par1 "                 QUOTE_N_END( "ABC" )
        "par2 "                 QUOTE_N_END( "12" )
        "par3 "                 QUOTE_N_END( "OFF" )
        "par5 "                 QUOTE_N_END( "OFF" )
        "par4 "                 QUOTE_N_END( "ON" )
        "NS1::par1 "            QUOTE_N_END( "12" )
        "NS2::NS3::par1 "       QUOTE_N_END( "OFF" )
        "NS2::NS4::par1 "       QUOTE_N_END( "ON" )
        "NS2::NS4::NS5::par1 "  QUOTE_N_END( "1 2 3" )

    ) );
}

//____________________________________________________________________________//

void
test_multipart_value()
{
    {
        param_namespace pn( "", NULL );
        BOOST_CHECK( !pn.load_parameters( "test_files/test_multipart_value1.cfg" ) );
    }

    {
        param_namespace pn( "", NULL );
        BOOST_CHECK( pn.load_parameters( "test_files/test_multipart_value2.cfg" ) );

        BOOST_CHECK_EQUAL( rtf::get_param_value( "a", pn ), "" );
    }


    {
        param_namespace pn( "", NULL );
        BOOST_CHECK( !pn.load_parameters( "test_files/test_multipart_value3.cfg" ) );
    }

    {
        param_namespace pn( "", NULL );
        BOOST_CHECK( pn.load_parameters( "test_files/test_multipart_value4.cfg" ) );

        BOOST_CHECK_EQUAL( rtf::get_param_value( "a", pn ), "\"" );
    }

    {
        param_namespace pn( "", NULL );
        BOOST_CHECK( !pn.load_parameters( "test_files/test_multipart_value5.cfg" ) );
    }

    {
        param_namespace pn( "", NULL );
        BOOST_CHECK( !pn.load_parameters( "test_files/test_multipart_value6.cfg" ) );
    }


    {
        param_namespace pn( "", NULL );
        BOOST_CHECK( !pn.load_parameters( "test_files/test_multipart_value7.cfg" ) );
    }

    {
        param_namespace pn( "", NULL );
        BOOST_CHECK( pn.load_parameters( "test_files/test_multipart_value8.cfg" ) );

        BOOST_CHECK_EQUAL( rtf::get_param_value( "a", pn ), "abcdef" );
    }

    {
        param_namespace pn( "", NULL );
        BOOST_CHECK( pn.load_parameters( "test_files/test_multipart_value9.cfg" ) );

        BOOST_CHECK_EQUAL( rtf::get_param_value( "a", pn ), "abcdef123" );
    }

    {
        param_namespace pn( "", NULL );
        BOOST_CHECK( !pn.load_parameters( "test_files/test_multipart_value10.cfg" ) );
    }

    {
        param_namespace pn( "", NULL );
        BOOST_CHECK( !pn.load_parameters( "test_files/test_multipart_value11.cfg" ) );
    }

    {
        param_namespace pn( "", NULL );
        BOOST_CHECK( !pn.load_parameters( "test_files/test_multipart_value12.cfg" ) );
    }

    {
        param_namespace pn( "", NULL );
        BOOST_CHECK( pn.load_parameters( "test_files/test_multipart_value13.cfg" ) );

        const_string pattern( "\"abc\"" );
        BOOST_CHECK_EQUAL( rtf::get_param_value( "a", pn ), pattern );
    }
}

//____________________________________________________________________________//

#endif

// EOF
