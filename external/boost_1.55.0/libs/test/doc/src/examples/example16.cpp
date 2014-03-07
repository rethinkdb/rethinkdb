#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test;

//____________________________________________________________________________//

void free_test_function()
{
    BOOST_CHECK( 2 == 1 );    
}

//____________________________________________________________________________//

test_suite*
init_unit_test_suite( int, char* [] ) {
    framework::master_test_suite().
        add( BOOST_TEST_CASE( &free_test_function ), 2 );

    return 0;
}

//____________________________________________________________________________//
