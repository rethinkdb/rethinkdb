#define BOOST_TEST_MODULE example
#include <boost/test/included/unit_test.hpp>
#include <boost/test/output_test_stream.hpp> 
using boost::test_tools::output_test_stream;

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test )
{
    output_test_stream output;
    int i=2;
    output << "i=" << i;
    BOOST_CHECK( !output.is_empty( false ) );
    BOOST_CHECK( output.check_length( 3, false ) );
    BOOST_CHECK( output.is_equal( "i=3" ) );
}

//____________________________________________________________________________//
