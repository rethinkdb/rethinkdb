/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <vector>
#include <algorithm>
#include <iostream>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/operator.hpp>

int
main()
{
    using boost::phoenix::arg_names::arg1;

    int init[] = { 2, 10, 4, 5, 1, 6, 8, 3, 9, 7 };
    std::vector<int> c(init, init + 10);
    typedef std::vector<int>::iterator iterator;

    //  Find the first odd number in container c
    iterator it = std::find_if(c.begin(), c.end(), arg1 % 2 == 1);

    if (it != c.end())
        std::cout << *it << std::endl;    //  if found, print the result
    return 0;
}
