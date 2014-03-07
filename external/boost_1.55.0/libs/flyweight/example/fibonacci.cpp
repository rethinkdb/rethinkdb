/* Boost.Flyweight example of flyweight-based memoization.
 *
 * Copyright 2006-2008 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/flyweight for library home page.
 */

#include <boost/flyweight.hpp>
#include <boost/flyweight/key_value.hpp>
#include <boost/flyweight/no_tracking.hpp>
#include <boost/noncopyable.hpp>
#include <iostream>

using namespace boost::flyweights;

/* Memoized calculation of Fibonacci numbers */

/* This class takes an int n and calculates F(n) at construction time */

struct compute_fibonacci;

/* A Fibonacci number can be modeled as a key-value flyweight
 * We choose the no_tracking policy so that the calculations
 * persist for future use throughout the program. See
 * Tutorial: Configuring Boost.Flyweight: Tracking policies for
 * further information on tracking policies.
 */

typedef flyweight<key_value<int,compute_fibonacci>,no_tracking> fibonacci;

/* Implementation of compute_fibonacci. Note that the construction
 * of compute_fibonacci(n) uses fibonacci(n-1) and fibonacci(n-2),
 * which effectively memoizes the computation.
 */

struct compute_fibonacci:private boost::noncopyable
{
  compute_fibonacci(int n):
    result(n==0?0:n==1?1:fibonacci(n-2).get()+fibonacci(n-1).get())
  {}

  operator int()const{return result;}
  int result;
};

int main()
{
  /* list some Fibonacci numbers */

  for(int n=0;n<40;++n){
    std::cout<<"F("<<n<<")="<<fibonacci(n)<<std::endl;
  }

  return 0;
}
