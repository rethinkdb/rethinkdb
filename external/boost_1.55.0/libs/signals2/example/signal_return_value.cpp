// Example program for returning a value from slots to signal invocation.
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

//[ signal_return_value_slot_defs_code_snippet
float product(float x, float y) { return x * y; }
float quotient(float x, float y) { return x / y; }
float sum(float x, float y) { return x + y; }
float difference(float x, float y) { return x - y; }
//]

int main()
{
  boost::signals2::signal<float (float x, float y)> sig;

//[ signal_return_value_main_code_snippet
  sig.connect(&product);
  sig.connect(&quotient);
  sig.connect(&sum);
  sig.connect(&difference);

  // The default combiner returns a boost::optional containing the return
  // value of the last slot in the slot list, in this case the
  // difference function.
  std::cout << *sig(5, 3) << std::endl;
//]
}

