#include <boost/test/included/unit_test.hpp>
#include <boost/test/parameterized_test.hpp>
using namespace boost::unit_test;

//____________________________________________________________________________//

void free_test_function( int i )
{
    BOOST_CHECK( i < 4 /* test assertion */ );
}

//____________________________________________________________________________//

test_suite*
init_unit_test_suite( int argc, char* argv[] )
{
    int params[] = { 1, 2, 3, 4, 5 };

    framework::master_test_suite().
        add( BOOST_PARAM_TEST_CASE( &free_test_function, params, params+5 ) );

    return 0;
}

//____________________________________________________________________________//
