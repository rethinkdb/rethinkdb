//  (C) Copyright Gennadiy Rozental 2002-2008.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision: 49313 $
//
//  Description : tests parameterized tests
// ***************************************************************************

// Boost.Test
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/test/parameterized_test.hpp>
#include <boost/test/unit_test_log.hpp>
#include <boost/test/results_collector.hpp>
#if BOOST_WORKAROUND(  __GNUC__, < 3 )
#include <boost/test/output_test_stream.hpp>
typedef boost::test_tools::output_test_stream onullstream_type;
#else
#include <boost/test/utils/nullstream.hpp>
typedef boost::onullstream onullstream_type;
#endif

namespace ut = boost::unit_test;

// BOOST
#include <boost/scoped_ptr.hpp>

// STL
#include <list>
#include <iostream>

//____________________________________________________________________________//

void test0( int i )
{
    BOOST_CHECK( i%2 == 0 );
}

//____________________________________________________________________________//

void test1( int i )
{
    BOOST_CHECK( i%2 == 0 );
    if( i%3 == 0 ) {
        throw 124;
    }
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test_case1 )
{
    onullstream_type    null_output;
    ut::test_suite* test = BOOST_TEST_SUITE( "" );

    ut::unit_test_log.set_stream( null_output );
    int test_data[] = { 2, 2, 2 };
    test->add( BOOST_PARAM_TEST_CASE( &test0, (int*)test_data, (int*)test_data + sizeof(test_data)/sizeof(int) ) );

    ut::framework::run( test );
    ut::test_results const& tr = ut::results_collector.results( test->p_id );

    ut::unit_test_log.set_stream( std::cout );
    BOOST_CHECK( tr.p_assertions_failed == 0 );
    BOOST_CHECK( !tr.p_aborted );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test_case2 )
{
    onullstream_type    null_output;
    ut::test_suite* test = BOOST_TEST_SUITE( "" );

    ut::unit_test_log.set_stream( null_output );
    int test_data[] = { 1, 2, 2 };    
    test->add( BOOST_PARAM_TEST_CASE( &test0, (int*)test_data, (int*)test_data + sizeof(test_data)/sizeof(int) ) );

    ut::framework::run( test );
    ut::test_results const& tr = ut::results_collector.results( test->p_id );

    ut::unit_test_log.set_stream( std::cout );
    BOOST_CHECK( tr.p_assertions_failed == 1 );
    BOOST_CHECK( !tr.p_aborted );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test_case3 )
{
    onullstream_type    null_output;
    ut::test_suite* test = BOOST_TEST_SUITE( "" );

    ut::unit_test_log.set_stream( null_output );
    int test_data[] = { 1, 1, 2 };    
    test->add( BOOST_PARAM_TEST_CASE( &test0, (int*)test_data, (int*)test_data + sizeof(test_data)/sizeof(int) ) );

    ut::framework::run( test );
    ut::test_results const& tr = ut::results_collector.results( test->p_id );

    ut::unit_test_log.set_stream( std::cout );
    BOOST_CHECK_EQUAL( tr.p_assertions_failed, (std::size_t)2 );
    BOOST_CHECK( !tr.p_aborted );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test_case4 )
{
    onullstream_type    null_output;
    ut::test_suite* test = BOOST_TEST_SUITE( "" );

    ut::unit_test_log.set_stream( null_output );
    int test_data[] = { 1, 1, 1 };    
    test->add( BOOST_PARAM_TEST_CASE( &test0, (int*)test_data, (int*)test_data + sizeof(test_data)/sizeof(int) ) );

    ut::framework::run( test );
    ut::test_results const& tr = ut::results_collector.results( test->p_id );

    ut::unit_test_log.set_stream( std::cout );
    BOOST_CHECK_EQUAL( tr.p_assertions_failed, (std::size_t)3 );
    BOOST_CHECK( !tr.p_aborted );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test_case5 )
{
    onullstream_type    null_output;
    ut::test_suite* test = BOOST_TEST_SUITE( "" );

    ut::unit_test_log.set_stream( null_output );
    int test_data[] = { 6, 6, 6 };    
    test->add( BOOST_PARAM_TEST_CASE( &test1, (int*)test_data, (int*)test_data + sizeof(test_data)/sizeof(int) ) );

    ut::framework::run( test );
    ut::test_results const& tr = ut::results_collector.results( test->p_id );

    ut::unit_test_log.set_stream( std::cout );
    BOOST_CHECK_EQUAL( tr.p_assertions_failed, (std::size_t)3 );
    BOOST_CHECK( !tr.p_aborted );
    BOOST_CHECK( !tr.passed() );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test_case6 )
{
    onullstream_type    null_output;
    ut::test_suite* test = BOOST_TEST_SUITE( "" );

    ut::unit_test_log.set_stream( null_output );
    int test_data[] = { 0, 3, 9 };    
    test->add( BOOST_PARAM_TEST_CASE( &test1, (int*)test_data, (int*)test_data + sizeof(test_data)/sizeof(int) ) );

    ut::framework::run( test );
    ut::test_results const& tr = ut::results_collector.results( test->p_id );

    ut::unit_test_log.set_stream( std::cout );
    BOOST_CHECK_EQUAL( tr.p_assertions_failed, (std::size_t)5 );
    BOOST_CHECK( !tr.p_aborted );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test_case7 )
{
    onullstream_type    null_output;
    ut::test_suite* test = BOOST_TEST_SUITE( "" );

    ut::unit_test_log.set_stream( null_output );
    int test_data[] = { 2, 3, 9 };
    test->add( BOOST_PARAM_TEST_CASE( &test1, (int*)test_data, (int*)test_data + sizeof(test_data)/sizeof(int) ) );

    ut::framework::run( test );
    ut::test_results const& tr = ut::results_collector.results( test->p_id );

    ut::unit_test_log.set_stream( std::cout );
    BOOST_CHECK_EQUAL( tr.p_assertions_failed, (std::size_t)4 );
    BOOST_CHECK( !tr.p_aborted );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test_case8 )
{
    onullstream_type    null_output;
    ut::test_suite* test = BOOST_TEST_SUITE( "" );

    ut::unit_test_log.set_stream( null_output );
    int test_data[] = { 3, 2, 6 };    
    test->add( BOOST_PARAM_TEST_CASE( &test1, (int*)test_data, (int*)test_data + sizeof(test_data)/sizeof(int) ) );

    ut::framework::run( test );
    ut::test_results const& tr = ut::results_collector.results( test->p_id );

    ut::unit_test_log.set_stream( std::cout );
    BOOST_CHECK_EQUAL( tr.p_assertions_failed, (std::size_t)3 );
    BOOST_CHECK( !tr.p_aborted );
}

//____________________________________________________________________________//

// EOF
