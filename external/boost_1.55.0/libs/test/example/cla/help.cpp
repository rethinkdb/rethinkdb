//  (C) Copyright Gennadiy Rozental 2001-2008.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

// Boost.Runtime.Param
#include <boost/test/utils/runtime/cla/named_parameter.hpp>
#include <boost/test/utils/runtime/cla/dual_name_parameter.hpp>
#include <boost/test/utils/runtime/cla/parser.hpp>

namespace rt  = boost::runtime;
namespace cla = boost::runtime::cla;

// STL
#include <iostream>

int main() {
    char* argv[] = { "/usr/local/bin/basic1", };
    int argc = sizeof(argv)/sizeof(char*);

    cla::parser P;

    try {
        P - (cla::prefix = "--", cla::separator = "=")
          << cla::dual_name_parameter<int>( "quiet|q" )      - (cla::description = "verbosity level")
          << cla::named_parameter<std::string>( "hostname" ) - (cla::guess_name, cla::description = "ABC server host name")
          << cla::named_parameter<int>( "port" )             - (cla::guess_name, cla::description = "ABC server port")
          << cla::dual_name_parameter<bool>( "help|?" )      - (cla::description = "this help message");

        P.parse( argc, argv );
    }
    catch( rt::logic_error const& ex ) {
        std::cout << "Logic error: " << ex.msg() << std::endl;
        P.help( std::cout );
        return -1;
    }

    return 0;
}

// EOF
