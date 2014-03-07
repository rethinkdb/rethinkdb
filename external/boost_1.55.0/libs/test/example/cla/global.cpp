//  (C) Copyright Gennadiy Rozental 2001-2008.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

// Boost.Runtime.Param
#include <boost/test/utils/runtime/cla/named_parameter.hpp>
#include <boost/test/utils/runtime/cla/positional_parameter.hpp>
#include <boost/test/utils/runtime/cla/parser.hpp>

namespace rt  = boost::runtime;
namespace cla = boost::runtime::cla;

// STL
#include <iostream>

int main() {
    char* argv[] = { "basic", "--abcd=25", "1.23", "--klmn", "wert" };
    int argc = sizeof(argv)/sizeof(char*);

    try {
        cla::parser P;
        float f;

        P - (cla::prefix = "--", cla::guess_name)
          << cla::named_parameter<int>( "abcd" ) - (cla::separator = "=")
          << cla::named_parameter<std::string>( "klmn" )
          << cla::positional_parameter<float>()  - (cla::assign_to = f);

        P.parse( argc, argv );

        std::cout << "abcd = " << P.get<int>( "abcd" ) << std::endl;
        std::cout << "klmn = " << P.get<std::string>( "klmn" ) << std::endl;
        std::cout << "f = " << f << std::endl;
    }
    catch( rt::logic_error const& ex ) {
        std::cout << "Logic error: " << ex.msg() << std::endl;
        return -1;
    }

    return 0;
}

// EOF
