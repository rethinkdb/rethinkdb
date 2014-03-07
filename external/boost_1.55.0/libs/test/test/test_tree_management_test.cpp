//  (C) Copyright Gennadiy Rozental 2007.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision: 57993 $
//
//  Description : test tree management unit test
// ***************************************************************************

// Boost.Test
#define BOOST_TEST_MODULE test tree management test
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test;

#include <boost/mpl/vector.hpp>

//____________________________________________________________________________//

// some empty test suites/cases

BOOST_AUTO_TEST_SUITE( S1 )

BOOST_AUTO_TEST_CASE_EXPECTED_FAILURES( tc1, 1 )

BOOST_AUTO_TEST_CASE( tc1 ) { BOOST_ERROR(""); }
BOOST_AUTO_TEST_CASE( tc2 ) {}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( S1 )

BOOST_AUTO_TEST_CASE( tc3 ) {}
BOOST_AUTO_TEST_CASE( tc4 ) {}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( S2 )

BOOST_AUTO_TEST_CASE( tc1 ) {}
BOOST_AUTO_TEST_CASE( tc2 ) {}

BOOST_AUTO_TEST_SUITE( S21 )

BOOST_AUTO_TEST_CASE_EXPECTED_FAILURES( tc1, 1 )

BOOST_AUTO_TEST_CASE( tc1 ) { BOOST_ERROR( "" ); }

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( S3 )

BOOST_AUTO_TEST_SUITE_END()

struct F1 {
   F1() { BOOST_TEST_MESSAGE( "In F1" ); }
};

BOOST_AUTO_TEST_SUITE( S4 )

typedef boost::mpl::vector<int,float,char> test_types;
BOOST_FIXTURE_TEST_CASE_TEMPLATE( tctempl, T, test_types, F1 )
{
}

BOOST_AUTO_TEST_SUITE_END()

struct F2 {
   F2() { BOOST_TEST_MESSAGE( "In F2" ); }
};

BOOST_FIXTURE_TEST_SUITE( S5, F2 )

typedef boost::mpl::vector<int,float,char,double,int> test_types;
BOOST_AUTO_TEST_CASE_TEMPLATE( tctempl, T, test_types )
{
}

BOOST_AUTO_TEST_SUITE_END()

//____________________________________________________________________________//

void empty() {}

BOOST_AUTO_TEST_CASE( manual_test_case_creation_test )
{
    test_case* tc1 = BOOST_TEST_CASE( &empty );

    BOOST_CHECK_EQUAL( tc1->p_type, tut_case );
    BOOST_CHECK_EQUAL( tc1->p_type_name, const_string( "case" ) );
    BOOST_CHECK_EQUAL( tc1->p_parent_id, 0U );
    BOOST_CHECK_NE( tc1->p_id, INV_TEST_UNIT_ID );

    BOOST_CHECK_EQUAL( tc1->p_expected_failures, 0U );
    BOOST_CHECK_EQUAL( tc1->p_timeout, 0U );
    BOOST_CHECK_EQUAL( tc1->p_name, const_string( "empty" ) );
    BOOST_CHECK( tc1->test_func() );
    BOOST_CHECK( tc1->p_enabled );

    BOOST_CHECK_EQUAL( &framework::get<test_case>( tc1->p_id ), tc1 );
    BOOST_CHECK_EQUAL( &framework::get( tc1->p_id, tut_case ), tc1 );

    BOOST_CHECK_THROW( &framework::get( tc1->p_id, tut_suite ), framework::internal_error );

    test_case* tc2 = make_test_case( &empty, "my test case" );
    BOOST_CHECK_EQUAL( tc2->p_name, const_string( "my test case" ) );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( manual_test_suite_creation )
{
    test_suite* ts1 = BOOST_TEST_SUITE( "TestSuite" );

    BOOST_CHECK_EQUAL( ts1->p_type, tut_suite );
    BOOST_CHECK_EQUAL( ts1->p_type_name, const_string( "suite" ) );
    BOOST_CHECK_EQUAL( ts1->p_parent_id, 0U );
    BOOST_CHECK_NE( ts1->p_id, INV_TEST_UNIT_ID );

    BOOST_CHECK_EQUAL( ts1->p_expected_failures, 0U );
    BOOST_CHECK_EQUAL( ts1->p_timeout, 0U );
    BOOST_CHECK_EQUAL( ts1->p_name, const_string( "TestSuite" ) );
    BOOST_CHECK( ts1->p_enabled );
    BOOST_CHECK_EQUAL( ts1->size(), 0U );

    BOOST_CHECK_EQUAL( &framework::get<test_suite>( ts1->p_id ), ts1 );
    BOOST_CHECK_EQUAL( &framework::get( ts1->p_id, tut_suite ), ts1 );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( manual_test_unit_registration )
{
    test_suite* ts1 = BOOST_TEST_SUITE( "TestSuite" );

    test_case* tc1 = make_test_case( &empty, "empty1" );

    ts1->add( tc1, 1, 10U );
    BOOST_CHECK_EQUAL( ts1->size(), 1U );

    BOOST_CHECK_EQUAL( tc1->p_expected_failures, 1U );
    BOOST_CHECK_EQUAL( tc1->p_timeout, 10U );
    BOOST_CHECK_EQUAL( ts1->p_expected_failures, 1U );

    test_case* tc2 = make_test_case( &empty, "empty2" );

    ts1->add( tc2, 2U );
    BOOST_CHECK_EQUAL( ts1->size(), 2U );

    BOOST_CHECK_EQUAL( tc2->p_expected_failures, 2U );
    BOOST_CHECK_EQUAL( tc2->p_timeout, 0U );
    BOOST_CHECK_EQUAL( ts1->p_expected_failures, 3U );

    test_suite* ts2 = BOOST_TEST_SUITE( "TestSuite2" );

    ts1->add( ts2 );
    BOOST_CHECK_EQUAL( ts1->size(), 3U );

    BOOST_CHECK_EQUAL( ts2->p_expected_failures, 0U );
    BOOST_CHECK_EQUAL( ts1->p_expected_failures, 3U );

    BOOST_CHECK_EQUAL( ts1->get( "empty1" ), tc1->p_id );
    BOOST_CHECK_EQUAL( ts1->get( "empty2" ), tc2->p_id );
    BOOST_CHECK_EQUAL( ts1->get( "TestSuite2" ), ts2->p_id );
    BOOST_CHECK_EQUAL( ts1->get( "another name" ), INV_TEST_UNIT_ID );

    ts1->remove( tc1->p_id );
    BOOST_CHECK_EQUAL( ts1->size(), 2U );
    BOOST_CHECK_EQUAL( ts1->get( "empty1" ), INV_TEST_UNIT_ID );

}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( automated_test_units_registration )
{
    test_suite& mts = framework::master_test_suite();

    BOOST_CHECK_EQUAL( mts.size(), 10U );
    BOOST_CHECK_EQUAL( mts.p_expected_failures, 2U );

    BOOST_CHECK_EQUAL( framework::get<test_case>( mts.get( "automated_test_units_registration" ) ).p_expected_failures, 0U );

    test_suite& S1 = framework::get<test_suite>( mts.get( "S1" ) );

    BOOST_CHECK_EQUAL( S1.size(), 4U );
    BOOST_CHECK_EQUAL( S1.p_expected_failures, 1U );

    test_suite& S2 = framework::get<test_suite>( mts.get( "S2" ) );

    BOOST_CHECK_EQUAL( S2.size(), 3U );
    BOOST_CHECK_EQUAL( S2.p_expected_failures, 1U );

    test_suite& S3 = framework::get<test_suite>( mts.get( "S3" ) );

    BOOST_CHECK_EQUAL( S3.size(), 0U );
    BOOST_CHECK_EQUAL( S3.p_expected_failures, 0U );

    test_suite& S21 = framework::get<test_suite>( S2.get( "S21" ) );

    BOOST_CHECK_EQUAL( S21.size(), 1U );
    BOOST_CHECK_EQUAL( S1.p_expected_failures, 1U );

    test_suite& S4 = framework::get<test_suite>( mts.get( "S4" ) );

    BOOST_CHECK_EQUAL( S4.size(), 3U );

    test_suite& S5 = framework::get<test_suite>( mts.get( "S5" ) );

    BOOST_CHECK_EQUAL( S5.size(), 5U );
}

//____________________________________________________________________________//

struct A {
    A() : i(0){}
    void test_methodA1() { i++; }
    void test_methodA2() { i++; }

    int i;
};

struct B : public A {
    void test_methodB() { i--; }
};

struct C {
    C() : i( 0 ) {}

    void virtual test_method() = 0;

    int i; 
};

struct D : public C {
    void virtual test_method() { i++; }
};

struct E : public C {
    void virtual test_method() { i+=2; }
};

BOOST_AUTO_TEST_CASE( user_class_test_case )
{
    boost::shared_ptr<A> instA( new A );
    test_case* tc1 = BOOST_CLASS_TEST_CASE( &A::test_methodA1, instA );
    test_case* tc2 = BOOST_CLASS_TEST_CASE( &A::test_methodA2, instA );

    BOOST_CHECK_EQUAL( tc1->p_name, const_string( "A::test_methodA1" ) );
    BOOST_CHECK_EQUAL( tc2->p_name, const_string( "A::test_methodA2" ) );

    BOOST_CHECK_EQUAL( instA->i, 0 );
    tc1->test_func()();
    BOOST_CHECK_EQUAL( instA->i, 1 );
    tc2->test_func()();
    BOOST_CHECK_EQUAL( instA->i, 2 );

    boost::shared_ptr<B> instB( new B );
    test_case* tc3 = BOOST_CLASS_TEST_CASE( &A::test_methodA1, instB );
    test_case* tc4 = BOOST_CLASS_TEST_CASE( &B::test_methodB, instB );

    BOOST_CHECK_EQUAL( tc3->p_name, const_string( "A::test_methodA1" ) );
    BOOST_CHECK_EQUAL( tc4->p_name, const_string( "B::test_methodB" ) );

    BOOST_CHECK_EQUAL( instB->i, 0 );
    tc3->test_func()();
    BOOST_CHECK_EQUAL( instB->i, 1 );
    tc4->test_func()();
    BOOST_CHECK_EQUAL( instB->i, 0 );

    boost::shared_ptr<C> instC1( new D );
    test_case* tc5 = BOOST_CLASS_TEST_CASE( &C::test_method, instC1 );

    BOOST_CHECK_EQUAL( tc5->p_name, const_string( "C::test_method" ) );

    tc5->test_func()();
    BOOST_CHECK_EQUAL( instC1->i, 1 );

    boost::shared_ptr<C> instC2( new E );
    test_case* tc6 = BOOST_CLASS_TEST_CASE( &C::test_method, instC2 );

    BOOST_CHECK_EQUAL( tc6->p_name, const_string( "C::test_method" ) );

    tc6->test_func()();
    BOOST_CHECK_EQUAL( instC2->i, 2 );
}

//____________________________________________________________________________//

// EOF
