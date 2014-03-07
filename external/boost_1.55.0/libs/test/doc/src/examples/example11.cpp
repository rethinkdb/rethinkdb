#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test;

//____________________________________________________________________________//

void test_case1() { /* : */ }
void test_case2() { /* : */ }
void test_case3() { /* : */ }
void test_case4() { /* : */ }

//____________________________________________________________________________//

test_suite*
init_unit_test_suite( int argc, char* argv[] )
{
    test_suite* ts1 = BOOST_TEST_SUITE( "test_suite1" );
    ts1->add( BOOST_TEST_CASE( &test_case1 ) );
    ts1->add( BOOST_TEST_CASE( &test_case2 ) );

    test_suite* ts2 = BOOST_TEST_SUITE( "test_suite2" );
    ts2->add( BOOST_TEST_CASE( &test_case3 ) );
    ts2->add( BOOST_TEST_CASE( &test_case4 ) );

    framework::master_test_suite().add( ts1 );
    framework::master_test_suite().add( ts2 );

    return 0;
}

//____________________________________________________________________________//
