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

// Boost.Runtime.Param
#include <boost/test/utils/runtime/env/environment.hpp>
namespace rt  = boost::runtime;
namespace env = boost::runtime::environment;

// Boost.Test
#include <boost/test/utils/basic_cstring/io.hpp>

// STL
#include <iostream>

int main() {
    try {
        std::cout << env::var("ABC").value<int>();
    }
    catch ( rt::logic_error const& ex ) {
        std::cout << ex.msg();
    }

    return 0;
}

// EOF

