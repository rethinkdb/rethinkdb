// Boost.Signals library

// Copyright Douglas Gregor 2001-2003. Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#include <boost/signals/signal2.hpp>
#include <iostream>

template<typename T>
struct first_positive {
  typedef T result_type;

  template<typename InputIterator>
  T operator()(InputIterator first, InputIterator last) const
  {
    while (first != last && !(*first > 0)) { ++first; }
    return (first == last) ? 0
                           : *first;
  }
};

template<typename T>
struct noisy_divide {
  typedef T result_type;

  T operator()(const T& x, const T& y) const
  {
    std::cout << "Dividing " << x << " and " << y << std::endl;
    return x/y;
  }
};

int main()
{
  boost::signal2<int, int, int, first_positive<int> > sig_positive;
  sig_positive.connect(std::plus<int>());
  sig_positive.connect(std::multiplies<int>());
  sig_positive.connect(std::minus<int>());
  sig_positive.connect(noisy_divide<int>());

  assert(sig_positive(3, -5) == 8); // returns 8, but prints nothing

  return 0;
}
