/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <vector>
#include <algorithm>
#include <iostream>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>

using namespace boost::phoenix;
using namespace boost::phoenix::arg_names;
using namespace boost::phoenix::arg_names;
using namespace std;

int
main()
{
    int init[] = { 2, 10, 4, 5, 1, 6, 8, 3, 9, 7 };
    vector<int> c(init, init + 10);
    typedef vector<int>::iterator iterator;

    //  Print all odd contents of an stl container c
    for_each(c.begin(), c.end(),
        if_(arg1 % 2 != 0)
        [
            cout << arg1 << ' '
        ]
    );

    return 0;
}
