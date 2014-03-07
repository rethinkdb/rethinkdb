//  (C) Copyright Gennadiy Rozental 2001-2008.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

// Boost.Runtime.Param
#include <boost/test/utils/runtime/cla/named_parameter.hpp>
#include <boost/test/utils/runtime/cla/parser.hpp>

namespace rt  = boost::runtime;
namespace cla = boost::runtime::cla;

// STL
#include <iostream>

int main() {
    char* argv[] = { "basic", "-abd", "as", "-cd", "aswdf", "--da", "25", "-ffd", "(fgh,", "asd)" };
    int argc = sizeof(argv)/sizeof(char*);

    try {
        cla::parser P;

        P - cla::ignore_mismatch 
          << cla::named_parameter<int>( "da" ) - ( cla::prefix = "--" )
          << cla::named_parameter<std::string>( "abd" );

        P.parse( argc, argv );

        std::cout << "da = " << P.get<int>( "da" ) << std::endl;
        std::cout << "abd = " << P.get<std::string>( "abd" ) << std::endl;

        std::cout << "argc = " << argc << std::endl;
        for( int i = 0; i<argc; ++i )
            std::cout << "argv[" << i << "]= " << argv[i] << std::endl;
    }
    catch( rt::logic_error const& ex ) {
        std::cout << "Logic error: " << ex.msg() << std::endl;
        return -1;
    }

    return 0;
}

// EOF
