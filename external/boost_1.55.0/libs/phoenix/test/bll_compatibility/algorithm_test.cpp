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

/*
#include "boost/lambda/lambda.hpp"
#include "boost/lambda/bind.hpp"
#include "boost/lambda/algorithm.hpp"
*/
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/operator.hpp>
#include <boost/phoenix/bind.hpp>
#include <boost/phoenix/scope.hpp>
#include <boost/phoenix/stl/algorithm/iteration.hpp>

#include <vector>
#include <map>
#include <set>
#include <string>

#include <iostream>

namespace phoenix = boost::phoenix;

void test_foreach() {

    using phoenix::placeholders::_1;
    using phoenix::ref;
    using phoenix::lambda;
  
  int a[10][20];
  int sum = 0;
        
  //for_each(arg1, for_each_tester())(array).value_;

  // Was:
  // std::for_each(a, a + 10, 
  //               bind(ll::for_each(), _1, _1 + 20, 
  //                    protect((_1 = var(sum), ++var(sum)))));
  // var replaced with ref, protect(..) replaced with lambda[..], no need for bind
  // phoenix algorithms are range based
  std::for_each(a, a + 10,
          phoenix::for_each(_1, lambda[_1 = ref(sum), ++ref(sum)]));
                /*phoenix::bind(phoenix::for_each, _1,
                            lambda[_1 = ref(sum), ++ref(sum)]));*/

  sum = 0;
  // Was:
  // std::for_each(a, a + 10, 
  //               bind(ll::for_each(), _1, _1 + 20, 
  //                 protect((sum += _1))));
  //
  std::for_each(a, a + 10, 
                phoenix::for_each( _1,
                     lambda[(ref(sum) += _1)]));

  BOOST_CHECK(sum == (199 + 1)/ 2 * 199);
}

// More tests needed (for all algorithms)

int test_main(int, char *[]) {

  test_foreach();

  return 0;
}






