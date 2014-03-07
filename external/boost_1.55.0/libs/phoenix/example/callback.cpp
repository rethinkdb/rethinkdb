/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <iostream>
#include <boost/phoenix/core.hpp>

template <typename F>
void print(F f)
{
    std::cout << f() << std::endl;
}

int
main()
{
    using boost::phoenix::val;

    print(val(3));
    print(val("Hello World"));
    return 0;
}
