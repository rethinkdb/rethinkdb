#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test;

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( free_test_function )
{
    BOOST_CHECK( true /* test assertion */ );
}

//____________________________________________________________________________//

test_suite*
init_unit_test_suite( int argc, char* argv[] ) 
{
    framework::master_test_suite().p_name.value = "my master test suite name";

    return 0;
}

//____________________________________________________________________________//
