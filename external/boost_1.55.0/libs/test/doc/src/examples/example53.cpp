#define BOOST_TEST_MODULE example
#include <boost/test/included/unit_test.hpp>

BOOST_AUTO_TEST_SUITE( test_suite )

BOOST_AUTO_TEST_CASE( test_case1 )
{
    BOOST_ERROR( "some error 1" );
}


BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_CASE( test_case_on_file_scope )
{
    BOOST_CHECK( true );
}

BOOST_AUTO_TEST_SUITE( test_suite )

BOOST_AUTO_TEST_CASE( test_case2 )
{
    BOOST_ERROR( "some error 2" );
}

BOOST_AUTO_TEST_SUITE_END()
