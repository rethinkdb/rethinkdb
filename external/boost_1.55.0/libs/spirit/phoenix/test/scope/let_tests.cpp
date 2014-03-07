/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <iostream>
#include <cmath>
#include <algorithm>
#include <vector>

#define PHOENIX_LIMIT 6

#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_scope.hpp>
#include <boost/spirit/include/phoenix_function.hpp>

using namespace boost::phoenix;
using namespace boost::phoenix::arg_names;
using namespace boost::phoenix::local_names;
using namespace std;

int
main()
{
    {
        int x = 1;
        BOOST_TEST(
            let(_a = _1)
            [
                _a
            ]
            (x) == x
        );
    }
    
    {
        int x = 1, y = 10;
        BOOST_TEST(
            let(_a = _1, _b = _2)
            [
                _a + _b
            ]
            (x, y) == x + y
        );
    }

    {
        int x = 1, y = 10, z = 13;
        BOOST_TEST(
            let(_x = _1, _y = _2)
            [
                let(_z = _3)
                [
                    _x + _y + _z
                ]
            ]
            (x, y, z) == x + y + z
        );
    }

    {
        int x = 1, y = 10;
        BOOST_TEST(
            let(_x = _1) 
            [
                _x +
                    let(_x = _2) 
                    [
                        -_x
                    ]
            ]
            (x, y) == x + -y
        );
    }
    
    {
        int x = 999;
        BOOST_TEST(
            let(_x = _1) // _x is a reference to _x
            [
                _x += 888
            ]
            (x) == 999 + 888
        );
        
        BOOST_TEST(x == 888 + 999);    
    }

    {
        int x = 999;
        BOOST_TEST(
            let(_x = val(_1)) // _x holds x by value 
            [
                val(_x += 888)
            ]
            (x) == x + 888
        );
        
        BOOST_TEST(x == 999);    
    }
    
    {
        BOOST_TEST(
            let(_a = 1, _b = 2, _c = 3, _d = 4, _e = 5)
            [
                _a + _b + _c + _d + _e
            ]
            () == 1 + 2 + 3 + 4 + 5
        );
    }
    
#ifdef PHOENIX_SHOULD_NOT_COMPILE_TEST
    {
        // disallow this:
        int i;
        (_a + _b)(i);
    }
#endif
    
    {
        // show that we can return a local from an outer scope
        int y = 0;
        int x = (let(_a = 1)[let(_b = _1)[ _a ]])(y);
        BOOST_TEST(x == 1);
    }

    {
        // show that this code returns an lvalue
        int i = 1;
        let(_a = arg1)[ _a ](i)++;
        BOOST_TEST(i == 2);
    }

    {
        // show that what you put in is what you get out
        int i = 1;
        int& j = let(_a = arg1)[ _a ](i);
        BOOST_TEST(&i == &j);
    }
    
    return boost::report_errors();
}

