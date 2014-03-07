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
    char* argv[] = { "basic", "-abc", "25" };
    int argc = 1;

    cla::parser P;

    try {
        P << cla::named_parameter<std::string>( "abcd" ) - cla::optional;

        P.parse( argc, argv );

        if( P["abcd"] )
            std::cout << "abcd = " << P.get<std::string>( "abcd" ) << std::endl;
        else
            std::cout << "not present" << std::endl;

        std::string s = P.get<std::string>( "abcd" );

    }
    catch( rt::logic_error const& ex ) {
        std::cout << "Logic error: " << ex.msg() << std::endl;
        return -1;
    }

    ///////////////////////////////////////////////////////////

    try {
        cla::parser P1;

        P1 << cla::named_parameter<std::string>( "abc" ) - cla::optional;

        argc = sizeof(argv)/sizeof(char*);

        P1.parse( argc, argv );

        if( P1["abc"] )
            std::cout << "abc = " << P1.get<std::string>( "abc" ) << std::endl;
        else
            std::cout << "not present" << std::endl;
    }
    catch( rt::logic_error const& ex ) {
        std::cout << "Logic error: " << ex.msg() << std::endl;
        return -1;
    }

    return 0;
}

// EOF
