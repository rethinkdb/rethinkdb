#include<boost/test/included/unit_test.hpp>

using namespace boost::unit_test; 

class test_class {
public:
    void test_method()
    {
        BOOST_CHECK( true /* test assertion */ );
    }
};

//____________________________________________________________________________//

void free_test_function() 
{ 
    test_class inst; 

    inst.test_method(); 
} 

//____________________________________________________________________________//

test_suite* 
init_unit_test_suite( int argc, char* argv[] ) 
{ 
    framework::master_test_suite().add( BOOST_TEST_CASE( &free_test_function ) ); 

    return 0; 
} 

//____________________________________________________________________________//
