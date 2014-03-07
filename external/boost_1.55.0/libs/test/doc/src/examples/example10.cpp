#define BOOST_TEST_MODULE example
#include <boost/test/included/unit_test.hpp>
#include <boost/mpl/list.hpp>

//____________________________________________________________________________//

typedef boost::mpl::list<int,long,unsigned char> test_types;

BOOST_AUTO_TEST_CASE_TEMPLATE( my_test, T, test_types )
{
    BOOST_CHECK_EQUAL( sizeof(T), (unsigned)4 )
}

//____________________________________________________________________________//
