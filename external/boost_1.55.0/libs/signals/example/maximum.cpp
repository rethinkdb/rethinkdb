// Boost.Signals library

// Copyright Douglas Gregor 2001-2003. Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#include <algorithm>
#include <iostream>
#include <boost/signals/signal2.hpp>

template<typename T>
struct maximum {
  typedef T result_type;

  template<typename InputIterator>
  T operator()(InputIterator first, InputIterator last) const
  {
    if (first == last)
      throw std::runtime_error("Cannot compute maximum of zero elements!");
    return *std::max_element(first, last);
  }
};

int main()
{
  boost::signal2<int, int, int, maximum<int> > sig_max;
  sig_max.connect(std::plus<int>());
  sig_max.connect(std::multiplies<int>());
  sig_max.connect(std::minus<int>());
  sig_max.connect(std::divides<int>());

  std::cout << sig_max(5, 3) << std::endl; // prints 15

  return 0;
}
