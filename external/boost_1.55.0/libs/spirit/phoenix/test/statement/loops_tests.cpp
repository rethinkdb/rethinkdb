/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <iostream>
#include <vector>
#include <algorithm>
#include <boost/detail/lightweight_test.hpp>
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
    vector<int> t = v;
    cout << endl;
    int x = 0;

    for_each(v.begin(), v.end(),
        (
            while_(arg1--)
            [
                cout << arg1 << ", ",
                ++ref(x)
            ],
            cout << val("\n")
        )
    );

    BOOST_TEST(x == 1+2+3+4+5+6+7+8+9+10);
    cout << endl;
    v = t;
    x = 0;

    for_each(v.begin(), v.end(),
        (
            do_
            [
                cout << arg1 << ", ",
                ++ref(x)
            ]
            .while_(arg1--),
            cout << val("\n")
        )
    );

    BOOST_TEST(x == 2+3+4+5+6+7+8+9+10+11);
    cout << endl;
    v = t;
    x = 0;

    int iii;
    for_each(v.begin(), v.end(),
        (
            for_(ref(iii) = 0, ref(iii) < arg1, ++ref(iii))
            [
                cout << arg1 << ", ",
                ++ref(x)
            ],
            cout << val("\n")
        )
    );

    BOOST_TEST(x == 1+2+3+4+5+6+7+8+9+10);
    cout << endl;
    v = t;
    x = 0;

    return boost::report_errors();
}
