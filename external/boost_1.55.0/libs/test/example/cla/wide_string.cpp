//  (C) Copyright Gennadiy Rozental 2001-2008.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

// Boost.Runtime.Param
#define BOOST_RT_PARAM_WIDE_STRING
#include <boost/test/utils/runtime/cla/named_parameter.hpp>
#include <boost/test/utils/runtime/cla/parser.hpp>

namespace rt  = boost::wide_runtime;
namespace cla = boost::wide_runtime::cla;

// STL
#include <iostream>

int main() {
    wchar_t* argv[] = { L"basic", L"-рсту", L"25" };
    int argc = sizeof(argv)/sizeof(char*);

    try {
        cla::parser P;

        P << cla::named_parameter<int>( L"рсту" );

        P.parse( argc, argv );

        std::wcout << L"рсту = " << P.get<int>( L"рсту" ) << std::endl;
    }
    catch( rt::logic_error const& ex ) {
        std::wcout << L"Logic Error: " << ex.msg() << std::endl;
        return -1;
    }

    return 0;
}

// EOF
