/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <iostream>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_algorithm.hpp>

using namespace boost::phoenix;
using namespace boost::phoenix::arg_names;

int
main()
{
    int array[] = {1, 2, 3};
    int output[3];
    copy(arg1, arg2)(array, output);
    return 0;
}
