/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <iostream>
#include <cmath>
#include <boost/detail/lightweight_test.hpp>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/operator.hpp>
#include <boost/phoenix/bind.hpp>

namespace phoenix = boost::phoenix;
using std::cout;
using std::pow;

    struct test
    {
        typedef void result_type;

        void
        operator()() const
        {
            cout << "Test lazy functions...\n";
        }
    };

    struct sqr
    {
        template <typename Sig>
        struct result;

        template <typename This, typename Arg>
        struct result<This(Arg&)>
        {
            typedef Arg type;
        };

        template <typename Arg>
        Arg
        operator()(Arg n) const
        {
            return n * n;
        }
    };

    struct fact
    {
        template <typename Sig>
        struct result;

        template <typename This, typename Arg>
        struct result<This(Arg&)>
        {
            typedef Arg type;
        };

        template <typename Arg>
        Arg
        operator()(Arg n) const
        {
            return (n <= 0) ? 1 : n * (*this)(n-1);
        }
    };

    struct power
    {
        template<typename Sig>
        struct result;

        template<typename This, typename Arg1, typename Arg2>
        struct result<This(Arg1&, Arg2&)>
        {
            typedef Arg1 type;
        };

        template <typename Arg1, typename Arg2>
        Arg1
        operator()(Arg1 a, Arg2 b) const
        {
            return pow(a, b);
        }
    };

    struct add
    {
        template <typename Sig>
        struct result;

        template <typename This, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
        struct result<This(Arg1&, Arg2&, Arg3&, Arg4&)>
        {
            typedef Arg1 type;
        };

        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
        Arg1
        operator()(Arg1 a, Arg2 b, Arg3 c, Arg4 d) const
        {
            return a + b + c + d;
        }
    };

int
main()
{
    using phoenix::bind;
    using phoenix::ref;
    using phoenix::arg_names::_1;
    using phoenix::arg_names::arg1;
    using phoenix::arg_names::arg2;

    int i5 = 5;
    double d5 = 5, d3 = 3;

    test()();
    BOOST_TEST(bind(sqr(), arg1)(i5) == (i5*i5));
    BOOST_TEST(bind(fact(), 4)() == 24);
    BOOST_TEST(bind(fact(), arg1)(i5) == 120);
    BOOST_TEST((int)bind(power(), arg1, arg2)(d5, d3) == (int)pow(d5, d3));
    BOOST_TEST((bind(sqr(), arg1) + 5)(i5) == ((i5*i5)+5));
    BOOST_TEST(bind(add(), arg1, arg1, arg1, arg1)(i5) == (5+5+5+5));

    int const ic5 = 5;
    // testing consts
    BOOST_TEST(bind(sqr(), arg1)(ic5) == (ic5*ic5));
    
    // From Steven Watanabe
    sqr s;
    int x = 2;
    int result = bind(ref(s), _1)(x);
    BOOST_TEST(result == 4);

    return boost::report_errors();
}
