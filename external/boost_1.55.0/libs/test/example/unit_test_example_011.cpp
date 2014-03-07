#define BOOST_TEST_MODULE broken
#include <boost/test/included/unit_test.hpp>


BOOST_AUTO_TEST_CASE( broken_test )
{
  ::system("ls");
}

