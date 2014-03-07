#include <stdexcept>
#include <boost/test/included/prg_exec_monitor.hpp> 

//____________________________________________________________________________//

int foo() { throw std::runtime_exception( "big trouble" ); }

//____________________________________________________________________________//

int cpp_main( int, char* [] ) // note the name
{
    foo();

    return 0;
}

//____________________________________________________________________________//
