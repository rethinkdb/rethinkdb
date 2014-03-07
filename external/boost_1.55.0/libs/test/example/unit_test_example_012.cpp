#include <boost/test/unit_test.hpp>
#include <cstdlib>

using boost::unit_test::test_suite;

void Vektor3Test1() { }

test_suite* Vektor3_test_suite()
{ 
  test_suite *test = BOOST_TEST_SUITE("Vektor3 test suite"); 
  test->add(BOOST_TEST_CASE(&Vektor3Test1));

  return test;
}

test_suite* init_unit_test_suite(int, char *[])
{ 
  std::system("true");
  // leads to "Test setup error: child has exited; pid: 1001; uid: 30540; exit value: 0"

  test_suite *test = BOOST_TEST_SUITE("Master test suite");
  test->add(Vektor3_test_suite());
  return test;
} 