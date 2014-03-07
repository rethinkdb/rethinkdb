#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test;

//____________________________________________________________________________//

void free_test_function()
{
    BOOST_CHECK( true /* test assertion */ );
}

//____________________________________________________________________________//

test_suite*
init_unit_test_suite( int argc, char* argv[] ) 
{
    if( framework::master_test_suite().argc > 1 )
        return 0;

    framework::master_test_suite().
        add( BOOST_TEST_CASE( &free_test_function ) );

    return 0;
}

//____________________________________________________________________________//

