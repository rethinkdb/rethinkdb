//  (C) Copyright Gennadiy Rozental 2003-2008.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision: 49313 $
//
//  Description : tests function template test case
// ***************************************************************************

// Boost.Test
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test_log.hpp>
#include <boost/test/results_collector.hpp>

#if BOOST_WORKAROUND(  __GNUC__, < 3 )
#include <boost/test/output_test_stream.hpp>
typedef boost::test_tools::output_test_stream onullstream_type;
#else
#include <boost/test/utils/nullstream.hpp>
typedef boost::onullstream onullstream_type;
#endif

// BOOST
#include <boost/mpl/range_c.hpp>
#include <boost/mpl/list_c.hpp>
#include <boost/scoped_ptr.hpp>

namespace ut = boost::unit_test;
namespace mpl = boost::mpl;

// STL
#include <iostream>

//____________________________________________________________________________//

BOOST_TEST_CASE_TEMPLATE_FUNCTION( test0, Number )
{
    BOOST_CHECK_EQUAL( 2, (int const)Number::value );
}

//____________________________________________________________________________//

BOOST_TEST_CASE_TEMPLATE_FUNCTION( test1, Number )
{
    BOOST_CHECK_EQUAL( 6, (int const)Number::value );
    BOOST_REQUIRE( 2 <= (int const)Number::value );
    BOOST_CHECK_EQUAL( 3, (int const)Number::value );
}

//____________________________________________________________________________//

BOOST_TEST_CASE_TEMPLATE_FUNCTION( test2, Number )
{
    throw Number();
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test0_only_2 )
{
    onullstream_type    null_output;

    typedef boost::mpl::list_c<int,2,2,2,2,2,2,2> only_2;
    ut::unit_test_log.set_stream( null_output );

    ut::test_suite* test = BOOST_TEST_SUITE( "" );
    test->add( BOOST_TEST_CASE_TEMPLATE( test0, only_2 ) );
    ut::framework::run( test );
    ut::test_results const& tr = ut::results_collector.results( test->p_id );

    ut::unit_test_log.set_stream( std::cout );
    BOOST_CHECK_EQUAL( tr.p_assertions_failed, (std::size_t)0 );
    BOOST_CHECK( !tr.p_aborted );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test0_one_to_ten )
{
    onullstream_type    null_output;
    ut::test_suite* test = BOOST_TEST_SUITE( "" );

    typedef boost::mpl::range_c<int,0,10> one_to_ten;
    ut::unit_test_log.set_stream( null_output );
    
    test->add( BOOST_TEST_CASE_TEMPLATE( test0, one_to_ten ) );

    ut::framework::run( test );
    ut::test_results const& tr = ut::results_collector.results( test->p_id );

    ut::unit_test_log.set_stream( std::cout );
    BOOST_CHECK_EQUAL( tr.p_assertions_failed, (std::size_t)9 );
    BOOST_CHECK( !tr.p_aborted );

}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test1_one_to_five )
{
    onullstream_type    null_output;
    ut::test_suite* test = BOOST_TEST_SUITE( "" );

    typedef boost::mpl::range_c<int,1,5> one_to_five;
    ut::unit_test_log.set_stream( null_output );
    test->add( BOOST_TEST_CASE_TEMPLATE( test1, one_to_five ) );

    ut::framework::run( test );
    ut::test_results const& tr = ut::results_collector.results( test->p_id );

    ut::unit_test_log.set_stream( std::cout );
    BOOST_CHECK_EQUAL( tr.p_assertions_failed, (std::size_t)7 );
    BOOST_CHECK( !tr.p_aborted );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test2_one_to_three )
{
    onullstream_type    null_output;
    ut::test_suite* test = BOOST_TEST_SUITE( "" );

    typedef boost::mpl::range_c<int,1,3> one_to_three;
    ut::unit_test_log.set_stream( null_output );
    test->add( BOOST_TEST_CASE_TEMPLATE( test2, one_to_three ) );

    ut::framework::run( test );
    ut::test_results const& tr = ut::results_collector.results( test->p_id );

    ut::unit_test_log.set_stream( std::cout );
    BOOST_CHECK_EQUAL( tr.p_assertions_failed, (std::size_t)2 );
    BOOST_CHECK( !tr.p_aborted );
    BOOST_CHECK( !tr.passed() );
}

//____________________________________________________________________________//

// EOF
