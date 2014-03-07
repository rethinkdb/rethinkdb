// Boost.Signals library

// Copyright Douglas Gregor 2001-2003. Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#include <iostream>
#include <boost/signals/signal2.hpp>

struct print_sum {
  void operator()(int x, int y) const { std::cout << x+y << std::endl; }
};

struct print_product {
  void operator()(int x, int y) const { std::cout << x*y << std::endl; }
};


int main()
{
  boost::signal2<void, int, int> sig;

  sig.connect(print_sum());
  sig.connect(print_product());

  sig(3, 5); // prints 8 and 15

  return 0;
}
