// Boost.Function library examples

//  Copyright Douglas Gregor 2001-2003. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#include <iostream>
#include <boost/function.hpp>

struct int_div {
  float operator()(int x, int y) const { return ((float)x)/y; };
};

int
main()
{
  boost::function<float (int, int)> f;
  f = int_div();

  std::cout << f(5, 3) << std::endl; // 1.66667

  return 0;
}
