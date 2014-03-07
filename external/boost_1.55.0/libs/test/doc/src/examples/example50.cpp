#define BOOST_TEST_MODULE example
#include <boost/test/included/unit_test.hpp>
#include <fstream>

//____________________________________________________________________________//

struct MyConfig {
    MyConfig() : test_log( "example.log" )  { boost::unit_test::unit_test_log.set_stream( test_log ); }
    ~MyConfig()                             { boost::unit_test::unit_test_log.set_stream( std::cout ); }

    std::ofstream test_log;
};

//____________________________________________________________________________//

BOOST_GLOBAL_FIXTURE( MyConfig );

BOOST_AUTO_TEST_CASE( test_case )
{
    BOOST_CHECK( false );
}

//____________________________________________________________________________//

