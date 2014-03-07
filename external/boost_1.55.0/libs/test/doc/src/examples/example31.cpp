#define BOOST_TEST_MODULE example
#include <boost/test/included/unit_test.hpp>

//____________________________________________________________________________//

boost::test_tools::predicate_result
compare_lists( std::list<int> const& l1, std::list<int> const& l2 )
{
    if( l1.size() != l2.size() ) {
        boost::test_tools::predicate_result res( false );

        res.message() << "Different sizes [" << l1.size() << "!=" << l2.size() << "]";

        return res;
    }

    return true;
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test_list_comparizon )
{
    std::list<int> l1, l2;
    l1.push_back( 1 );
    l1.push_back( 2 );

    BOOST_CHECK( compare_lists( l1, l2 ) );
}

//____________________________________________________________________________//

