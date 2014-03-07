#define BOOST_TEST_MODULE example
#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test;

//____________________________________________________________________________//

struct MyConfig {
    MyConfig()  { unit_test_log.set_format( XML ); }
    ~MyConfig() {}
};

//____________________________________________________________________________//

BOOST_GLOBAL_FIXTURE( MyConfig );

BOOST_AUTO_TEST_CASE( test_case0 )
{
    BOOST_CHECK( false );
}

//____________________________________________________________________________//

