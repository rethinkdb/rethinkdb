/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <iostream>
#include <cmath>
#include <algorithm>
#include <vector>

#include <typeinfo>

#include <boost/detail/lightweight_test.hpp>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/operator.hpp>
#include <boost/phoenix/function.hpp>
#include <boost/phoenix/bind.hpp>
#include <boost/phoenix/scope.hpp>

namespace boost { namespace phoenix
{
    struct for_each_impl
    {
        template <typename Sig>
        struct result;

        template <typename This, typename C, typename F>
        struct result<This(C,F)>
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
        template <typename Sig>
        struct result;

        template <typename This, typename C, typename T>
        struct result<This(C,T)>
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

struct zzz {};

int
main()
{
    using boost::phoenix::lambda;
    using boost::phoenix::let;
    using boost::phoenix::ref;
    using boost::phoenix::val;
    using boost::phoenix::arg_names::_1;
    using boost::phoenix::arg_names::_2;
    using boost::phoenix::local_names::_a;
    using boost::phoenix::local_names::_b;
    using boost::phoenix::placeholders::arg1;

    {
        int x = 1;
        int y = lambda[_1]()(x);
        BOOST_TEST(x == y);
    }

    {
        int x = 1, y = 10;
        BOOST_TEST(
            (_1 + lambda[_1 + 2])(x)(y) == 1+10+2
        );
        BOOST_TEST(
            (_1 + lambda[-_1])(x)(y) == 1+-10
        );
    }

    {
        int x = 1, y = 10, z = 13;
        BOOST_TEST(
            lambda(_a = _1, _b = _2)
            [
                _1 + _a + _b
            ]
            (x, z)(y) == x + y + z
        );
    }

    {
        int x = 4;
        int y = 5;
        lambda(_a = _1)[_a = 555](x)();
        BOOST_TEST(x == 555);
        (void)y;
    }

    {
        int x = 1;
        long x2 = 2;
        short x3 = 3;
        char const* y = "hello";
        zzz z;

        BOOST_TEST(lambda[_1](x)(y) == y);
        BOOST_TEST(lambda(_a = _1)[_a](x)(y) == x);
        BOOST_TEST(lambda(_a = _1)[lambda[_a]](x)(y)(z) == x);
        BOOST_TEST(lambda(_a = _1)[lambda[_a + _1]](x)(y)(x) == 2);
        BOOST_TEST(lambda(_a = _1)[lambda(_b = _1)[_a + _b + _1]](x)(x2)(x3) == 6);
    }

    {
        int x = 1, y = 10;
        BOOST_TEST(
            (_1 + lambda(_a = _1)[_a + _1 + 2])(x)(y) == 1+1+10+2
        );
    }

    {
        int x = 1, y = 10;
        BOOST_TEST(
        (
                _1 + 
                lambda(_a = _1)
                [
                    _a + lambda[_a + 2]
                ]
            )
            (x)(y)(y) == 1+1+1+2
        );
    }

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

    {
        int x = 1, y = 10, z = 13;
    
        BOOST_TEST(
            lambda(_a = _1, _b = _2)
            [
                _1 + _a + _b
            ]
            (x, z)(y) == x + y + z
        );
    }

    {
		  {
            // $$$ Fixme. This should not be failing $$$
            //int x = (let(_a = lambda[val(1)])[_a])()();
            //BOOST_TEST(x == 1);
		  }

		  {
            int x = (let(_a = lambda[val(1)])[bind(_a)])();
            BOOST_TEST(x == 1);
		  }
    }

    {
        int i = 0;
        int j = 2;
        BOOST_TEST(lambda[let(_a = _1)[_a = _2]]()(i, j) == j);
        BOOST_TEST(i == j);
    }

    return boost::report_errors();
}

