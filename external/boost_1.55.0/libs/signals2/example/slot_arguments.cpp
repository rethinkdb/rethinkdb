// Example program for passing arguments from signal invocations to slots.
//
// Copyright Douglas Gregor 2001-2004.
// Copyright Frank Mori Hess 2009.
//
// Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// For more information, see http://www.boost.org

#include <iostream>
#include <boost/signals2/signal.hpp>

//[ slot_arguments_slot_defs_code_snippet
void print_args(float x, float y)
{
  std::cout << "The arguments are " << x << " and " << y << std::endl;
}

void print_sum(float x, float y)
{
  std::cout << "The sum is " << x + y << std::endl;
}

void print_product(float x, float y)
{
  std::cout << "The product is " << x * y << std::endl;
}

void print_difference(float x, float y)
{
  std::cout << "The difference is " << x - y << std::endl;
}

void print_quotient(float x, float y)
{
  std::cout << "The quotient is " << x / y << std::endl;
}
//]

int main()
{
//[ slot_arguments_main_code_snippet
  boost::signals2::signal<void (float, float)> sig;

  sig.connect(&print_args);
  sig.connect(&print_sum);
  sig.connect(&print_product);
  sig.connect(&print_difference);
  sig.connect(&print_quotient);

  sig(5., 3.);
//]
  return 0;
}

