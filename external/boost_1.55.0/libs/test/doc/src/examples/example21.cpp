#define BOOST_TEST_MODULE example
#include <boost/test/included/unit_test.hpp>

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test_init )
{
    int current_time = 0; // real call is required here

    BOOST_TEST_MESSAGE( "Testing initialization :" );
    BOOST_TEST_MESSAGE( "Current time:" << current_time );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test_update )
{
    std::string field_name = "Volume";
    int         value      = 100;

    BOOST_TEST_MESSAGE( "Testing update :" );
    BOOST_TEST_MESSAGE( "Update " << field_name << " with " << value );
}

//____________________________________________________________________________//
