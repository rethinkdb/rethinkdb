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
    using boost::phoenix::ref;

    int i = 3;
    char const* s = "Hello World";
    std::cout << ref(i)() << std::endl;
    std::cout << ref(s)() << std::endl;
    return 0;
}
