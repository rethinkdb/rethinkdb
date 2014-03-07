/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <iostream>
#include <boost/phoenix/core.hpp>

int
main()
{
    using boost::phoenix::arg_names::arg1;
    using boost::phoenix::arg_names::arg2;

    int i = 3;
    char const* s = "Hello World";
    std::cout << arg1(i) << std::endl;        // prints 3
    std::cout << arg2(i, s) << std::endl;     // prints "Hello World"
    return 0;
}
