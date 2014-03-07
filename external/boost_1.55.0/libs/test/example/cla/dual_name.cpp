//  (C) Copyright Gennadiy Rozental 2001-2008.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

// Boost.Runtime.Param
#include <boost/test/utils/runtime/cla/dual_name_parameter.hpp>
#include <boost/test/utils/runtime/cla/parser.hpp>

namespace rt  = boost::runtime;
namespace cla = boost::runtime::cla;

// STL
#include <iostream>

int main() {
    char* argv[] = { "basic", "/abcd", "25", "-k=1" };
    int argc = sizeof(argv)/sizeof(char*);

    try {
        cla::parser P;

        P << cla::dual_name_parameter<int>( "abcd|a" ) - (cla::prefix = "/|-")
          << cla::dual_name_parameter<int>()           - (cla::name = "klmn|k", cla::separator = "|=");

        P.parse( argc, argv );

        std::cout << "abcd = " << P.get<int>( "abcd" ) << std::endl;

        std::cout << "klmn = " << P.get<int>( "k" ) << std::endl;
    }
    catch( rt::logic_error const& ex ) {
        std::cout << "Logic error: " << ex.msg() << std::endl;
        return -1;
    }

    return 0;
}

// EOF
