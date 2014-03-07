//  (C) Copyright Gennadiy Rozental 2001-2008.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

// Boost.Runtime.Param
#include <boost/test/utils/runtime/cla/positional_parameter.hpp>
#include <boost/test/utils/runtime/cla/parser.hpp>

namespace rt  = boost::runtime;
namespace cla = boost::runtime::cla;

// STL
#include <iostream>

int main() {
    char* argv[] = { "basic", "--klmn-- 14" };
    int argc = sizeof(argv)/sizeof(char*);

    try {
        cla::parser P;
        int i;

        P << cla::positional_parameter<std::string>( "abcd" ) 
          << cla::positional_parameter<int>()                   - (cla::assign_to = i)
          << cla::positional_parameter<std::string>( "klmn" )   - (cla::default_value = std::string("file"))
          << cla::positional_parameter<std::string>( "oprt" )   - cla::optional;

        P.parse( argc, argv );

        std::cout << "abcd = " << P.get<std::string>( "abcd" ) << std::endl;
        std::cout << "i = " << i << std::endl;
        std::cout << "klmn = " << P.get<std::string>( "klmn" ) << std::endl;
        std::cout << "oprt " << (P["oprt"] ? "present" : "not present" ) << std::endl;
    }
    catch( rt::logic_error const& ex ) {
        std::cout << "Logic error: " << ex.msg() << std::endl;
        return -1;
    }

    return 0;
}

// EOF
