// Boost.Function library examples

//  Copyright Douglas Gregor 2001-2003. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#include <iostream>
#include <boost/function.hpp>
#include <functional>

struct X {
  X(int val) : value(val) {}

  int foo(int x) { return x * value; }

  int value;
};


int
main()
{
  boost::function<int (int)> f;
  X x(7);
  f = std::bind1st(std::mem_fun(&X::foo), &x);

  std::cout << f(5) << std::endl; // Call x.foo(5)
  return 0;
}
