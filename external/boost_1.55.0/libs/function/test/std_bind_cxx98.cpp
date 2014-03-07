// Function library

// Copyright (C) 2001-2003 Douglas Gregor

// Use, modification and distribution is subject to the Boost Software 
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt) 

// For more information, see http://www.boost.org/

    
#include <boost/function.hpp>
#include <iostream>
#include <functional>

struct X {
  int foo(int);
};
int X::foo(int x) { return -x; }

int main()
{
      boost::function<int (int)> f;
  X x;
  f = std::bind1st(
        std::mem_fun(&X::foo), &x);
  f(5); // Call x.foo(5)

    return 0;
}
