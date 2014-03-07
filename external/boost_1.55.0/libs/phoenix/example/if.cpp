/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <iostream>
#include <vector>
#include <algorithm>
#include <boost/phoenix/statement.hpp>
#include <boost/phoenix/operator.hpp>
#include <boost/phoenix/core.hpp>

int
main()
{
    using boost::phoenix::if_;
    using boost::phoenix::arg_names::arg1;

    int init[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    std::vector<int> v(init, init+10);

    std::cout << std::dec;
    int x = 0;

    std::for_each(v.begin(), v.end(),
        if_(arg1 > 5)
        [
            std::cout << arg1 << ", "
        ]
    );

    std::cout << std::endl;

    return 0;
}
