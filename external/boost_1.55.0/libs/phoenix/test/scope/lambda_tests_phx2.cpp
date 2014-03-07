/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <iostream>
#include <cmath>
#include <algorithm>
#include <vector>

#include <boost/detail/lightweight_test.hpp>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/operator.hpp>
#include <boost/phoenix/scope.hpp>
#include <boost/phoenix/function.hpp>

namespace boost { namespace phoenix
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
}}

using namespace boost::phoenix;
using namespace boost::phoenix::arg_names;
using namespace boost::phoenix::local_names;
using namespace std;

struct zzz {};

int
main()
{
    {
        using boost::phoenix::for_each;

        int init[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        std::vector<int> v(init, init+10);

        int x = 0;
        for_each(_1, lambda(_a = _2)[_a += _1])(v, x);
        BOOST_TEST(x == 55);
    }

    {
        using boost::phoenix::for_each;
        using boost::phoenix::push_back;

        int x = 10;
        std::vector<std::vector<int> > v(10);

        for_each(_1, lambda(_a = _2)[push_back(_1, _a)])(v, x);

        int y = 0;
        for_each(arg1, lambda[ref(y) += _1[0]])(v);
        BOOST_TEST(y == 100);
    }
    return boost::report_errors();
}

