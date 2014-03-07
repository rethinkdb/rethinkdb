/*=============================================================================
    Phoenix V1.2.1
    Copyright (c) 2001-2003 Joel de Guzman

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <vector>
#include <algorithm>
#include <iostream>
#include <cassert>

#define PHOENIX_LIMIT 15
#include <boost/spirit/include/phoenix1_operators.hpp>
#include <boost/spirit/include/phoenix1_primitives.hpp>
#include <boost/spirit/include/phoenix1_composite.hpp>
#include <boost/spirit/include/phoenix1_special_ops.hpp>
#include <boost/spirit/include/phoenix1_statements.hpp>
#include <boost/spirit/include/phoenix1_functions.hpp>
#include <boost/spirit/include/phoenix1_closures.hpp>

//////////////////////////////////
using namespace std;
using namespace phoenix;

//////////////////////////////////
int
main()
{
    struct my_closure : closure<int, string, double> {

        member1 num;
        member2 message;
        member3 real;
    };

    my_closure clos;

    {   //  First stack frame
        closure_frame<my_closure::self_t> frame(clos);
        (clos.num = 123)();
        (clos.num += 456)();
        (clos.real = clos.num / 56.5)();
        (clos.message = "Hello " + string("World "))();

        {   //  Second stack frame
            closure_frame<my_closure::self_t> frame(clos);
            (clos.num = 987)();
            (clos.message = "Abracadabra ")();
            (clos.real = clos.num * 1e30)();

            {   //  Third stack frame
                tuple<int, char const*, double> init(-1, "Direct Init ", 3.14);
                closure_frame<my_closure::self_t> frame(clos, init);

                (cout << clos.message << clos.num << ", " << clos.real << '\n')();
            }

            (cout << clos.message << clos.num << ", " << clos.real << '\n')();
        }

        (cout << clos.message << clos.num << ", " << clos.real << '\n')();
    }

    return 0;
}
