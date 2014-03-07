// Boost.Function library

//  Copyright Douglas Gregor 2002-2003. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#include <iostream>
#include <cstdlib>

#include <boost/test/minimal.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/function.hpp>

static unsigned
func_impl(int arg1, bool arg2, double arg3)
{
  using namespace std;
  return abs (static_cast<int>((arg2 ? arg1 : 2 * arg1) * arg3));
}

int test_main(int, char*[])
{
  using boost::function;
  using namespace boost::lambda;

  function <unsigned(bool, double)> f1 = bind(func_impl, 15, _1, _2);
  function <unsigned(double)>       f2 = boost::lambda::bind(f1, false, _1);
  function <unsigned()>             f3 = boost::lambda::bind(f2, 4.0);

  f3();

  return 0;
}

