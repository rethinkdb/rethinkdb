// Boost.Signals library

// Copyright Douglas Gregor 2001-2003. Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#include <iostream>
#include <boost/signals/signal2.hpp>
#include <string>
#include <functional>

void print_sum(int x, int y)
{
  std::cout << "Sum = " << x+y << std::endl;
}

void print_product(int x, int y)
{
  std::cout << "Product = " << x*y << std::endl;
}

void print_quotient(float x, float y)
{
  std::cout << "Quotient = " << x/y << std::endl;
}

int main()
{
  typedef boost::signal2<void, int, int, boost::last_value<void>,
                         std::string, std::less<std::string>,
                         void (*)(int, int)> sig_type;

  sig_type sig;
  sig.connect(&print_sum);
  sig.connect(&print_product);

  sig(3, 5); // print sum and product of 3 and 5

  // should fail
  //   sig.connect(&print_quotient);
}
