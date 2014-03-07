/*=============================================================================
    Copyright (c) 2011 Thomas Heller
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#include <boost/phoenix/core.hpp>
#include <boost/phoenix/statement.hpp>
#include <boost/phoenix/operator.hpp>
#include <boost/phoenix/scope.hpp>
#include <iostream>
#include <boost/phoenix/scope/this.hpp>


template <typename T0>
void f(T0 t)
{
    std::cout << t(1) << "\n";
    std::cout << t(2) << "\n";
    std::cout << t(3) << "\n";
    std::cout << t(4) << "\n";
    std::cout << t(5) << "\n";
    std::cout << t(6) << "\n";
    std::cout << t(7) << "\n";
}

template <typename T0>
void f_2(T0 t)
{
    for(int i = 0; i < 10; ++i)
    {
        for(int j = 0; j < i; ++j)
        {
            std::cout << t(i, j) << " ";
        }
        std::cout << "\n";
    }
}

int main()
{
    //using boost::phoenix::_this;
    using boost::phoenix::if_;
    using boost::phoenix::if_else;
    using boost::phoenix::val;
    using boost::phoenix::let;
    using boost::phoenix::nothing;
    using boost::phoenix::arg_names::_1;
    using boost::phoenix::arg_names::_2;
    using boost::phoenix::local_names::_a;

    f((
        if_(_1 == 0)
        [
            std::cout << val("\n")
        ]
        .else_
        [
            std::cout << _1 << " "
          , this_(_1 - 1)
        ]
		 , val(0)
    ));

	 /*
	 f((
        if_else(
            _1 == 0
          , _1
          ,this_(_1 - 1)
        )
    ));
	 */
	 
	 f((
        if_else(
            _1 != 0
          ,this_(_1 - 1)
          , _1
        )
    ));
/* 

    f(( // fac(n) = n * fac(n-1); fac(1) = 1
        if_else(
            _1 <= 1
          , 1
          , _1 * _this(_1 - 1)
        )
    ));
    
    f(( // fac(n) = n * fac(n-1); fac(1) = 1
        if_else(
            _1 > 1
          , let(_a = _this(_1-1))[_1 * _a]
          , 1
        )
    ));
    
    f(( // fib(n) = fib(n-1) + fib(n-2); fib(0) = 0; fib(1) = 1
        if_else(
            _1 == 0
          , 0
          , if_else(
                _1 == 1
              , 1
              , _this(_1 - 1) + _this(_1 - 2)
            )
        )
    ));

    f_2(( // bin(n, k) = bin(n-1, k-1) + bin(n-1, k); bin(n, 0) = 1; bin(0, k) = 0
        if_else(
            _1 == 0
          , 0
          , if_else(
                _2 == 0
              , 1
              , _this(_1 - 1, _2 - 1) + _this(_1 - 1, _2)
            )
        )
    ));
	 */
}
