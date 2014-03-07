/*=============================================================================
    Copyright (c) 2001-2003 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <vector>
#include <algorithm>
#include <iostream>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/function.hpp>

struct factorial_impl
{
    template <typename Sig>
    struct result;
    
    template <typename This, typename Arg>
    struct result<This(Arg)>
        : result<This(Arg const &)>
    {};

    template <typename This, typename Arg>
    struct result<This(Arg &)>
    {
        typedef Arg type;
    };

    template <typename Arg>
    Arg operator()(Arg n) const
    {
        return (n <= 0) ? 1 : n * this->operator()(n-1);
    }
};


int
main()
{
    using boost::phoenix::arg_names::arg1;
    boost::phoenix::function<factorial_impl> factorial;
    int i = 4;
    std::cout << factorial(i)() << std::endl;
    std::cout << factorial(arg1)(i) << std::endl;
    return 0;
}
