/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <iostream>
#include <vector>
#include <algorithm>
#include <boost/detail/lightweight_test.hpp>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/statement.hpp>
#include <boost/phoenix/operator.hpp>

int
main()
{
    using boost::phoenix::arg_names::arg1;
    using boost::phoenix::if_;
    using boost::phoenix::ref;
    
    int init[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    std::vector<int> v(init, init+10);

    std::cout << std::dec;
    int x = 0;

    for_each(v.begin(), v.end(),
        if_(arg1 > 3 && arg1 <= 8)
        [
            std::cout << arg1 << ", ",
            ref(x) += arg1
        ]
    );

    std::cout << std::endl;
    BOOST_TEST(x == 4+5+6+7+8);

    x = 0;
    int y = 0;
    int z = 0;

    for_each(v.begin(), v.end(),
        if_(arg1 > 5)
        [
            std::cout << arg1 << " > 5\n",
            ref(x) += arg1
        ]
        .else_
        [
            if_(arg1 == 5)
            [
                std::cout << arg1 << " == 5\n",
                ref(z) += arg1
            ]
            .else_
            [
                std::cout << arg1 << " < 5\n",
                ref(y) += arg1
            ]
        ]
    );

    std::cout << std::endl;
    BOOST_TEST(x == 6+7+8+9+10);
    BOOST_TEST(y == 1+2+3+4);
    BOOST_TEST(z == 5);

    return boost::report_errors();
}
