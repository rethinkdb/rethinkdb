/*=============================================================================
    Phoenix V1.2.1
    Copyright (c) 2001-2003 Joel de Guzman

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <iostream>
#include <vector>
#include <algorithm>
#include <boost/detail/lightweight_test.hpp>

#define PHOENIX_LIMIT 15
#include <boost/spirit/include/phoenix1_primitives.hpp>
#include <boost/spirit/include/phoenix1_functions.hpp>
#include <boost/spirit/include/phoenix1_operators.hpp>
#include <boost/spirit/include/phoenix1_special_ops.hpp>
#include <boost/spirit/include/phoenix1_statements.hpp>

using namespace phoenix;
using namespace std;

    ///////////////////////////////////////////////////////////////////////////////
    struct poly_print_ {

        template <typename ArgT>
        struct result { typedef void type; };

        template <typename ArgT>
        void operator()(ArgT v) const
        { cout << v; }
    };

    function<poly_print_> poly_print;

///////////////////////////////////////////////////////////////////////////////
int
main()
{
    char    c1 = '1';
    int     i1 = 1;
    double  d2_5 = 2.5;
    string hello = "hello";
    const char* world = " world";

///////////////////////////////////////////////////////////////////////////////
//
//  Block statements
//
///////////////////////////////////////////////////////////////////////////////
    (
        poly_print(arg1),
        poly_print(arg2),
        poly_print(arg3),
        poly_print(arg4),
        poly_print(arg5),
        poly_print("\n\n")
    )
    (hello, c1, world, i1, d2_5);

///////////////////////////////////////////////////////////////////////////////
//
//  If-else, while, do-while, for tatements
//
///////////////////////////////////////////////////////////////////////////////

    vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    v.push_back(4);
    v.push_back(5);
    v.push_back(6);
    v.push_back(7);
    v.push_back(8);
    v.push_back(9);
    v.push_back(10);

    cout << dec;

    //////////////////////////////////
    for_each(v.begin(), v.end(),
        if_(arg1 > 3 && arg1 <= 8)
        [
            cout << arg1 << ", "
        ]
    );

    cout << endl;

    //////////////////////////////////
    for_each(v.begin(), v.end(),
        if_(arg1 > 5)
        [
            cout << arg1 << " > 5\n"
        ]
        .else_
        [
            if_(arg1 == 5)
            [
                cout << arg1 << " == 5\n"
            ]
            .else_
            [
                cout << arg1 << " < 5\n"
            ]
        ]
    );

    cout << endl;

    vector<int> t = v;
    //////////////////////////////////
    for_each(v.begin(), v.end(),
        (
            while_(arg1--)
            [
                cout << arg1 << ", "
            ],
            cout << val("\n")
        )
    );

    v = t;
    cout << endl;

    //////////////////////////////////
    for_each(v.begin(), v.end(),
        (
            do_
            [
                cout << arg1 << ", "
            ]
            .while_(arg1--),
            cout << val("\n")
        )
    );

    v = t;
    cout << endl;

    //////////////////////////////////
    int iii;
    for_each(v.begin(), v.end(),
        (
            for_(var(iii) = 0, var(iii) < arg1, ++var(iii))
            [
                cout << arg1 << ", "
            ],
            cout << val("\n")
        )
    );

    v = t;
    cout << endl;

///////////////////////////////////////////////////////////////////////////////
//
//  End asserts
//
///////////////////////////////////////////////////////////////////////////////

    return boost::report_errors();    
}
