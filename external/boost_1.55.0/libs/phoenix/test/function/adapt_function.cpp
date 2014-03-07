/*=============================================================================
  Copyright (c) 2011 Thomas Heller

  Distributed under the Boost Software License, Version 1.0. (See accompanying 
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
  ==============================================================================*/
#include <iostream>
#include <cmath>
#include <boost/detail/lightweight_test.hpp>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/function.hpp>

namespace impl
{
    void
    test()
    {
        std::cout << "Test adapting functions...\n";
    }

    int
    negate(int n)
    {
        return -n;
    }

    int
    plus(int a, int b)
    {
        return a + b;
    }

    template <typename T>
    T
    plus(T a, T b, T c)
    {
        return a + b + c;
    }

    int
    plus4(int a, int b, int c, int d)
    {
        return a + b + c + d;
    }
}
    
BOOST_PHOENIX_ADAPT_FUNCTION_NULLARY(void, test, impl::test)
BOOST_PHOENIX_ADAPT_FUNCTION(int, negate, impl::negate, 1)
BOOST_PHOENIX_ADAPT_FUNCTION(int, plus, impl::plus, 2)
BOOST_PHOENIX_ADAPT_FUNCTION(
    typename boost::remove_reference<A0>::type
  , plus
  , impl::plus
  , 3
)
BOOST_PHOENIX_ADAPT_FUNCTION(int, plus4, impl::plus4, 4)

int
main()
{
    using boost::phoenix::arg_names::arg1;
    using boost::phoenix::arg_names::arg2;

    int a = 123;
    int b = 256;

    test()();
    BOOST_TEST(::negate(arg1)(a) == -a);
    BOOST_TEST(::plus(arg1, arg2)(a, b) == a+b);
    BOOST_TEST(::plus(arg1, arg2, 3)(a, b) == a+b+3);
    BOOST_TEST(plus4(arg1, arg2, 3, 4)(a, b) == a+b+3+4);

    return boost::report_errors();
}
