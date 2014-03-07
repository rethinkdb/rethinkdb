// Boost.Function library examples

//  Copyright Douglas Gregor 2001-2003. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#include <iostream>
#include <boost/function.hpp>

void do_sum_avg(int values[], int n, int& sum, float& avg)
{
  sum = 0;
  for (int i = 0; i < n; i++)
    sum += values[i];
  avg = (float)sum / n;
}

int
main()
{
  // The second parameter should be int[], but some compilers (e.g., GCC)
  // complain about this
  boost::function<void (int*, int, int&, float&)> sum_avg;

  sum_avg = &do_sum_avg;

  int values[5] = { 1, 1, 2, 3, 5 };
  int sum;
  float avg;
  sum_avg(values, 5, sum, avg);

  std::cout << "sum = " << sum << std::endl;
  std::cout << "avg = " << avg << std::endl;
  return 0;
}
