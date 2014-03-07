/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <iostream>
#include <algorithm>
#include <vector>

#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_scope.hpp>
#include <boost/spirit/include/phoenix_function.hpp>

using namespace boost::phoenix;
using namespace boost::phoenix::arg_names;
using namespace boost::phoenix::local_names;
using namespace std;

namespace lazy_stuff
{
    struct for_each_impl
    {
        template <typename C, typename F>
        struct result
        {
            typedef void type;
        };

        template <typename C, typename F>
        void operator()(C& c, F f) const
        {
            std::for_each(c.begin(), c.end(), f);
        }
    };

    function<for_each_impl> const for_each = for_each_impl();

    struct push_back_impl
    {
        template <typename C, typename T>
        struct result
        {
            typedef void type;
        };

        template <typename C, typename T>
        void operator()(C& c, T& x) const
        {
            c.push_back(x);
        }
    };

    function<push_back_impl> const push_back = push_back_impl();
}

int
main()
{
    {
        using lazy_stuff::for_each;
        using lazy_stuff::push_back;

        int x = 10;
        std::vector<std::vector<int> > v(10);

        for_each(arg1, 
            lambda(_a = arg2)
            [
                push_back(arg1, _a)
            ]
        )
        (v, x);
    }

    return 0;
}

