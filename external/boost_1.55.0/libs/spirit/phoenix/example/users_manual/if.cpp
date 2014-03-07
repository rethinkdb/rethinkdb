/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <iostream>
#include <vector>
#include <algorithm>
#include <boost/spirit/include/phoenix_statement.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_core.hpp>

using namespace boost::phoenix;
using namespace boost::phoenix::arg_names;
using namespace std;

int
main()
{
    int init[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    vector<int> v(init, init+10);

    cout << dec;
    int x = 0;

    for_each(v.begin(), v.end(),
        if_(arg1 > 5)
        [
            cout << arg1 << ", "
        ]
    );

    return 0;
}
