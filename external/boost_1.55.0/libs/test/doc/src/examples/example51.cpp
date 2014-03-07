#define BOOST_TEST_MODULE example
#include <boost/test/included/unit_test.hpp>
#include <boost/test/detail/unit_test_parameters.hpp>
using namespace boost::unit_test;

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test_case0 )
{
    if( runtime_config::log_level() < log_warnings )
        unit_test_log.set_threshold_level( log_warnings );

    BOOST_WARN( sizeof(int) > 4 );
}

//____________________________________________________________________________//

