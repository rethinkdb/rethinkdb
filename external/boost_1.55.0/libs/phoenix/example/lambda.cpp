/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <iostream>
#include <algorithm>
#include <vector>

#include <boost/phoenix/scope.hpp>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/operator.hpp>
#include <boost/phoenix/function.hpp>

namespace lazy_stuff
{
    using boost::phoenix::function;

    struct for_each_impl
    {
        typedef void result_type;

        template <typename C, typename F>
        void operator()(C& c, F f) const
        {
            std::for_each(c.begin(), c.end(), f);
        }
    };

    function<for_each_impl> const for_each = for_each_impl();

    struct push_back_impl
    {
        typedef void result_type;

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

        using boost::phoenix::lambda;
        using boost::phoenix::arg_names::arg1;
        using boost::phoenix::arg_names::arg2;
        using boost::phoenix::local_names::_a;

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

