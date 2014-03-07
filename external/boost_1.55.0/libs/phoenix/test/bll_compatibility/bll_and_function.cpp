//  bll_and_function.cpp  - The Boost Lambda Library -----------------------
//
// Copyright (C) 2000-2003 Jaakko Jarvi (jaakko.jarvi@cs.utu.fi)
// Copyright (C) 2000-2003 Gary Powell (powellg@amazon.com)
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org

// test using BLL and boost::function

#include <boost/test/minimal.hpp>    // see "Header Implementation Option"

#include "boost/lambda/lambda.hpp"

#include "boost/function.hpp"

#include <vector>
#include <map>
#include <set>
#include <string>


using namespace boost::lambda;

using namespace std;

void test_function() {

  boost::function<int (int, int)> f;
  f = _1 + _2;

 BOOST_CHECK(f(1, 2)== 3);

 int i=1; int j=2;
 boost::function<int& (int&, int)> g = _1 += _2;
 g(i, j);
 BOOST_CHECK(i==3);



  int* sum = new int();
  *sum = 0;
  boost::function<int& (int)> counter = *sum += _1;
  counter(5); // ok, sum* = 5;
  BOOST_CHECK(*sum == 5);
  delete sum; 
  
  // The next statement would lead to a dangling reference
  // counter(3); // error, *sum does not exist anymore

}


int test_main(int, char *[]) {

  test_function();

  return 0;
}






