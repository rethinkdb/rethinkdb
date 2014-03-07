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

void
named_integer( rt::cstring source, boost::optional<int>& value )
{
    if( source == "one" )
        value = 1;
    else if( source == "two" )
        value = 2;
    else
        value = 0;
}

env::variable<int> NUMBER1( "NUMBER1", env::interpreter = &named_integer );

int main() {

    std::cout << NUMBER1 << std::endl;

    std::cout << env::var<int>( "NUMBER2", ( env::interpreter = &named_integer, env::default_value = 10 ))
              << std::endl;

    return 0;
}

// EOF

