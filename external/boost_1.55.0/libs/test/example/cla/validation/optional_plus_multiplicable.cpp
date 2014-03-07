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
    {
        char* argv[] = { "basic" };
        int argc = sizeof(argv)/sizeof(char*);

        try {
            cla::parser P;
            P << cla::named_parameter<int>( "abcd" ) - (cla::optional, cla::multiplicable);
            P.parse( argc, argv );

            std::cout << "abcd = " << (P["abcd"] ? "present" : "not present" );
            std::cout << std::endl;
        }
        catch( rt::logic_error const& ex ) {
            std::cout << "Logic error: " << ex.msg() << std::endl;
            return -1;
        }
    }

    {
        char* argv[] = { "basic", "-abcd", "25" };
        int argc = sizeof(argv)/sizeof(char*);

        try {
            cla::parser P;
            P << cla::named_parameter<int>( "abcd" ) - (cla::optional, cla::multiplicable);
            P.parse( argc, argv );

            std::list<int> arg_values = P.get<std::list<int> >( "abcd" );

            std::cout << "abcd = ";
            std::copy( arg_values.begin(), arg_values.end(), std::ostream_iterator<int>( std::cout, " " ) );
            std::cout << std::endl;
        }
        catch( rt::logic_error const& ex ) {
            std::cout << "Logic error: " << ex.msg() << std::endl;
            return -1;
        }
    }

    {
        char* argv[] = { "basic", "-abcd", "25", "-abcd", "50" };
        int argc = sizeof(argv)/sizeof(char*);

        try {
            cla::parser P;
            P << cla::named_parameter<int>( "abcd" ) - (cla::optional, cla::multiplicable);
            P.parse( argc, argv );

            std::list<int> arg_values = P.get<std::list<int> >( "abcd" );

            std::cout << "abcd = ";
            std::copy( arg_values.begin(), arg_values.end(), std::ostream_iterator<int>( std::cout, " " ) );
            std::cout << std::endl;
        }
        catch( rt::logic_error const& ex ) {
            std::cout << "Logic error: " << ex.msg() << std::endl;
            return -1;
        }
    }

    return 0;
}
