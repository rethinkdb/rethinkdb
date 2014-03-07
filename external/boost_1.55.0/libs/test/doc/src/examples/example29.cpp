#define BOOST_TEST_MODULE example
#include <boost/test/included/unit_test.hpp>
#include <boost/test/output_test_stream.hpp> 
using boost::test_tools::output_test_stream;

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test )
{
    output_test_stream output( "pattern_file", true );
    int i=2;
    output << "i=" << i;
    BOOST_CHECK( output.match_pattern() );

    output << "\nFile: " << __FILE__ << " Line: " << __LINE__;
    BOOST_CHECK( output.match_pattern() );
}

//____________________________________________________________________________//
