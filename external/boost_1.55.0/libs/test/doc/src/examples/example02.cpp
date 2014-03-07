#include <boost/test/included/unit_test.hpp>
#include <boost/bind.hpp>
using namespace boost::unit_test;

//____________________________________________________________________________//

class test_class {
public:
    void test_method()
    {
        BOOST_CHECK( true /* test assertion */ );
    }
} tester;

//____________________________________________________________________________//

test_suite*
init_unit_test_suite( int argc, char* argv[] ) 
{
    framework::master_test_suite().
        add( BOOST_TEST_CASE( boost::bind( &test_class::test_method, &tester )));

    return 0;
}

//____________________________________________________________________________//
