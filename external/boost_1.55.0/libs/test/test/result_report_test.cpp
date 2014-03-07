//  (C) Copyright Gennadiy Rozental 2001-2008.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision: 57993 $
//
//  Description : tests Unit Test Framework reporting facilities against
//  pattern file
// ***************************************************************************

// Boost.Test
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/test/results_reporter.hpp>
#include <boost/test/output_test_stream.hpp>
#include <boost/test/unit_test_log.hpp>
#include <boost/test/unit_test_suite.hpp>
#include <boost/test/framework.hpp>
#include <boost/test/detail/unit_test_parameters.hpp>
#if BOOST_WORKAROUND(  __GNUC__, < 3 )
typedef boost::test_tools::output_test_stream onullstream_type;
#else
#include <boost/test/utils/nullstream.hpp>
typedef boost::onullstream onullstream_type;
#endif

// BOOST
#include <boost/lexical_cast.hpp>

// STL
#include <iostream>

using boost::test_tools::output_test_stream;
using namespace boost::unit_test;

//____________________________________________________________________________//

void good_foo() {}

void bad_foo()  { 
    onullstream_type null_out;
    unit_test_log.set_stream( null_out );
    BOOST_ERROR( "" );
    unit_test_log.set_stream( std::cout );
}

struct log_guard {
    ~log_guard()
    {
        unit_test_log.set_stream( std::cout );
    }
};

void very_bad_foo()  { 
    log_guard lg;
    ut_detail::ignore_unused_variable_warning( lg );
    onullstream_type null_out;
    unit_test_log.set_stream( null_out );
    BOOST_FAIL( "" );
}

//____________________________________________________________________________//

void check( output_test_stream& output, output_format report_format, test_unit_id id )
{
    results_reporter::set_format( report_format );

    results_reporter::confirmation_report( id );
    output << "*************************************************************************\n";
    BOOST_CHECK( output.match_pattern() );

    results_reporter::short_report( id );
    output << "*************************************************************************\n";
    BOOST_CHECK( output.match_pattern() );

    results_reporter::detailed_report( id );
    output << "*************************************************************************\n";
    BOOST_CHECK( output.match_pattern() );
}

//____________________________________________________________________________//

void check( output_test_stream& output, test_unit_id id )
{
    check( output, CLF, id );
    check( output, XML, id );
}

//____________________________________________________________________________//

struct guard {
    ~guard()
    {
        results_reporter::set_stream( std::cerr );
        results_reporter::set_format( runtime_config::report_format() );
    }
};

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test_result_reports ) 
{
    guard G;
    ut_detail::ignore_unused_variable_warning( G );

#define PATTERN_FILE_NAME "result_report_test.pattern"

    std::string pattern_file_name(
        framework::master_test_suite().argc == 1 
            ? (runtime_config::save_pattern() ? PATTERN_FILE_NAME : "./test_files/" PATTERN_FILE_NAME )
            : framework::master_test_suite().argv[1] );

    output_test_stream test_output( pattern_file_name, !runtime_config::save_pattern() );
    results_reporter::set_stream( test_output );

    test_suite* ts_0 = BOOST_TEST_SUITE( "0 test cases inside" );
    
    test_suite* ts_1 = BOOST_TEST_SUITE( "1 test cases inside" );
        ts_1->add( BOOST_TEST_CASE( good_foo ) );

    test_suite* ts_1b = BOOST_TEST_SUITE( "1 bad test case inside" );
        ts_1b->add( BOOST_TEST_CASE( bad_foo ), 1 );

    test_suite* ts_2 = BOOST_TEST_SUITE( "2 test cases inside" );
        ts_2->add( BOOST_TEST_CASE( good_foo ) );
        ts_2->add( BOOST_TEST_CASE( bad_foo ), 1 );

    test_suite* ts_3 = BOOST_TEST_SUITE( "3 test cases inside" );
        ts_3->add( BOOST_TEST_CASE( bad_foo ) );
        test_case* tc1 = BOOST_TEST_CASE( very_bad_foo );
        ts_3->add( tc1 );
        test_case* tc2 = BOOST_TEST_CASE( bad_foo );
        ts_3->add( tc2 );
        tc2->depends_on( tc1 );

    test_suite* ts_main = BOOST_TEST_SUITE( "Fake Test Suite Hierarchy" );
        ts_main->add( ts_0 );
        ts_main->add( ts_1 );
        ts_main->add( ts_2 );
        ts_main->add( ts_3 );

    framework::run( ts_1 );
    check( test_output, ts_1->p_id );

    framework::run( ts_1b );
    check( test_output, ts_1b->p_id );

    framework::run( ts_2 );
    check( test_output, ts_2->p_id );

    framework::run( ts_3 );
    check( test_output, ts_3->p_id );

    ts_3->depends_on( tc1 );

    framework::run( ts_main );
    check( test_output, ts_main->p_id );

    results_reporter::set_stream( std::cout );
}

//____________________________________________________________________________//

// EOF
