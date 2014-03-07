// Function library

// Copyright (C) 2001-2003 Douglas Gregor

// Use, modification and distribution is subject to the Boost Software 
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt) 

// For more information, see http://www.boost.org/

    
#include <boost/function.hpp>
#include <iostream>

void do_sum_avg(int values[], int n, int& sum, float& avg)
{
  sum = 0;
  for (int i = 0; i < n; i++)
    sum += values[i];
  avg = (float)sum / n;
}
int main()
{
    boost::function<void (int values[], int n, int& sum, float& avg)> sum_avg;
    sum_avg = &do_sum_avg;

    return 0;
}
