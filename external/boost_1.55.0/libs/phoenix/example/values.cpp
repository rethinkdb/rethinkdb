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
    using boost::phoenix::val;

    std::cout << val(3)() << std::endl;
    std::cout << val("Hello World")() << std::endl;
    return 0;
}
