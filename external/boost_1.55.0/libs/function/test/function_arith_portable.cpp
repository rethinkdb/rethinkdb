// Function library

// Copyright (C) 2001-2003 Douglas Gregor

// Use, modification and distribution is subject to the Boost Software 
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt) 

// For more information, see http://www.boost.org/

    
#include <boost/function.hpp>
#include <iostream>

float mul_ints(int x, int y) { return ((float)x) * y; }
struct int_div { 
  float operator()(int x, int y) const { return ((float)x)/y; }; 
};
int main()
{
    boost::function2<float, int, int> f;
    f = int_div();
    std::cout << f(5, 3) << std::endl;
    if (f)
  std::cout << f(5, 3) << std::endl;
else
  std::cout << "f has no target, so it is unsafe to call" << std::endl;
    f = 0;
    f = &mul_ints;

    return 0;
}
