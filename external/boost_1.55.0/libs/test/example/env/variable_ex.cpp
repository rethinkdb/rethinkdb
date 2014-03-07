//  (C) Copyright Gennadiy Rozental 2001-2008.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

// Boost.Runtime.Param
#include <boost/test/utils/runtime/env/variable.hpp>
#include <boost/test/utils/basic_cstring/io.hpp>

namespace rt  = boost::runtime;
namespace env = boost::runtime::env;

// STL
#include <iostream>

env::variable<> TEMP( "TEMP" );

env::variable<int> ProcNumber( "NUMBER_OF_PROCESSORS" );

env::variable<int> abc( "abccccc" );

void print_int( env::variable_base const& var ) {
    std::cout << var.name() << (var.has_value() ? " is present: " : " is not present: " ) << var.value<int>() << std::endl;
}

int main() {
    std::cout << TEMP << '\n' << ProcNumber << std::endl;

    rt::cstring val = TEMP.value();
    std::cout << " val="  << val << std::endl;

    int n = ProcNumber.value();

    std::cout << " n="  << n << std::endl;

    boost::optional<int> opt_n;
    ProcNumber.value( opt_n );

    std::cout << " n="  << opt_n << std::endl;

    print_int( ProcNumber );

    if( ProcNumber == 1 )
        std::cout << "ProcNumber = 1\n";

    if( 2 != ProcNumber )
        std::cout << "ProcNumber != 2\n";

    if( abc != 1 )
        std::cout << "abc != 1\n";

    abc = 1;

    if( abc == 1 )
        std::cout << "abc == 1\n";

    TEMP = "../tmp";

    std::cout << TEMP << std::endl;

    return 0;
}

// EOF

