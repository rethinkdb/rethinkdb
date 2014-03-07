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

void
named_integer( rt::cla::argv_traverser& source, boost::optional<int>& value )
{
    if( source.token() == "one" ) {
        value = 1;
        source.next_token();
    }
    else if( source.token() == "two" ) {
        value = 2;
        source.next_token();
    }
    else
        value = 0;
}

int main() {
    char* argv[] = { "basic", "-abcd", "one" };
    int argc = sizeof(argv)/sizeof(char*);

    try {
        cla::parser P;

        P << cla::named_parameter<int>( "abcd" ) - (cla::interpreter = &named_integer);

        P.parse( argc, argv );

        std::cout << "abcd = " << P.get<int>( "abcd" ) << std::endl;
    }
    catch( rt::logic_error const& ex ) {
        std::cout << "Logic error: " << ex.msg() << std::endl;
        return -1;
    }

    return 0;
}

// EOF
