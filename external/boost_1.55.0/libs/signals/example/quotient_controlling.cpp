// Boost.Signals library

// Copyright Douglas Gregor 2001-2003. Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#include <iostream>
#include <boost/signals/signal2.hpp>
#include <cassert>

struct print_sum {
  void operator()(int x, int y) const { std::cout << x+y << std::endl; }
};

struct print_product {
  void operator()(int x, int y) const { std::cout << x*y << std::endl; }
};

struct print_difference {
  void operator()(int x, int y) const { std::cout << x-y << std::endl; }
};

struct print_quotient {
  void operator()(int x, int y) const { std::cout << x/-y << std::endl; }
};


int main()
{
  boost::signal2<void, int, int> sig;

  sig.connect(print_sum());
  sig.connect(print_product());

  sig(3, 5);

  boost::signals::connection print_diff_con = sig.connect(print_difference());

  // sig is still connected to print_diff_con
  assert(print_diff_con.connected());

  sig(5, 3); // prints 8, 15, and 2

  print_diff_con.disconnect(); // disconnect the print_difference slot

  sig(5, 3); // now prints 8 and 15, but not the difference

  assert(!print_diff_con.connected()); // not connected any more

  {
    boost::signals::scoped_connection c = sig.connect(print_quotient());
    sig(5, 3); // prints 8, 15, and 1
  } // c falls out of scope, so sig and print_quotient are disconnected

  sig(5, 3); // prints 8 and 15

  return 0;
}
