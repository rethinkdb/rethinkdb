//  bind_tests_simple.cpp  -- The Boost Lambda Library ------------------
//
// Copyright (C) 2000-2003 Jaakko Jarvi (jaakko.jarvi@cs.utu.fi)
// Copyright (C) 2000-2003 Gary Powell (powellg@amazon.com)
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org

// -----------------------------------------------------------------------


#include <boost/test/minimal.hpp>    // see "Header Implementation Option"

#include "boost/lambda/bind.hpp"

#include <iostream>

using namespace boost::lambda;


int sum_of_args_0() { return 0; }
int sum_of_args_1(int a) { return a; }
int sum_of_args_2(int a, int b) { return a+b; }
int sum_of_args_3(int a, int b, int c) { return a+b+c; }
int sum_of_args_4(int a, int b, int c, int d) { return a+b+c+d; }
int sum_of_args_5(int a, int b, int c, int d, int e) { return a+b+c+d+e; }
int sum_of_args_6(int a, int b, int c, int d, int e, int f) { return a+b+c+d+e+f; }
int sum_of_args_7(int a, int b, int c, int d, int e, int f, int g) { return a+b+c+d+e+f+g; }
int sum_of_args_8(int a, int b, int c, int d, int e, int f, int g, int h) { return a+b+c+d+e+f+g+h; }
int sum_of_args_9(int a, int b, int c, int d, int e, int f, int g, int h, int i) { return a+b+c+d+e+f+g+h+i; }


// ----------------------------

class A {
  int i; 
public:
  A(int n) : i(n) {};
  int add(const int& j) { return i + j; }
};

void test_member_functions()
{
  using boost::ref;
  A a(10);
  int i = 1;

  BOOST_CHECK(bind(&A::add, ref(a), _1)(i) == 11);
  BOOST_CHECK(bind(&A::add, &a, _1)(i) == 11);
  BOOST_CHECK(bind(&A::add, _1, 1)(a) == 11);
  BOOST_CHECK(bind(&A::add, _1, 1)(make_const(&a)) == 11);

  // This should fail, as lambda functors store arguments as const
  // bind(&A::add, a, _1); 
}

int test_main(int, char *[]) {

  int i = 1; int j = 2; int k = 3;
  int result;
   

  // bind all parameters
  BOOST_CHECK(bind(sum_of_args_0)()==0);
  BOOST_CHECK(bind(sum_of_args_1, 1)()==1);
  BOOST_CHECK(bind(sum_of_args_2, 1, 2)()==3);
  BOOST_CHECK(bind(sum_of_args_3, 1, 2, 3)()==6);
  BOOST_CHECK(bind(sum_of_args_4, 1, 2, 3, 4)()==10);
  BOOST_CHECK(bind(sum_of_args_5, 1, 2, 3, 4, 5)()==15);
  BOOST_CHECK(bind(sum_of_args_6, 1, 2, 3, 4, 5, 6)()==21);   
  BOOST_CHECK(bind(sum_of_args_7, 1, 2, 3, 4, 5, 6, 7)()==28);   
  BOOST_CHECK(bind(sum_of_args_8, 1, 2, 3, 4, 5, 6, 7, 8)()==36);   
  BOOST_CHECK(bind(sum_of_args_9, 1, 2, 3, 4, 5, 6, 7, 8, 9)()==45);      

  // first parameter open
  BOOST_CHECK(bind(sum_of_args_0)()==0);
  BOOST_CHECK(bind(sum_of_args_1, _1)(i)==1);
  BOOST_CHECK(bind(sum_of_args_2, _1, 2)(i)==3);
  BOOST_CHECK(bind(sum_of_args_3, _1, 2, 3)(i)==6);
  BOOST_CHECK(bind(sum_of_args_4, _1, 2, 3, 4)(i)==10);
  BOOST_CHECK(bind(sum_of_args_5, _1, 2, 3, 4, 5)(i)==15);
  BOOST_CHECK(bind(sum_of_args_6, _1, 2, 3, 4, 5, 6)(i)==21);   
  BOOST_CHECK(bind(sum_of_args_7, _1, 2, 3, 4, 5, 6, 7)(i)==28);   
  BOOST_CHECK(bind(sum_of_args_8, _1, 2, 3, 4, 5, 6, 7, 8)(i)==36);   
  BOOST_CHECK(bind(sum_of_args_9, _1, 2, 3, 4, 5, 6, 7, 8, 9)(i)==45);      

  // two open arguments 
  BOOST_CHECK(bind(sum_of_args_0)()==0);
  BOOST_CHECK(bind(sum_of_args_1, _1)(i)==1);
  BOOST_CHECK(bind(sum_of_args_2, _1, _2)(i, j)==3);
  BOOST_CHECK(bind(sum_of_args_3, _1, _2, 3)(i, j)==6);
  BOOST_CHECK(bind(sum_of_args_4, _1, _2, 3, 4)(i, j)==10);
  BOOST_CHECK(bind(sum_of_args_5, _1, _2, 3, 4, 5)(i, j)==15);
  BOOST_CHECK(bind(sum_of_args_6, _1, _2, 3, 4, 5, 6)(i, j)==21);   
  BOOST_CHECK(bind(sum_of_args_7, _1, _2, 3, 4, 5, 6, 7)(i, j)==28);   
  BOOST_CHECK(bind(sum_of_args_8, _1, _2, 3, 4, 5, 6, 7, 8)(i, j)==36);   
  BOOST_CHECK(bind(sum_of_args_9, _1, _2, 3, 4, 5, 6, 7, 8, 9)(i, j)==45);      

  // three open arguments 
  BOOST_CHECK(bind(sum_of_args_0)()==0);
  BOOST_CHECK(bind(sum_of_args_1, _1)(i)==1);
  BOOST_CHECK(bind(sum_of_args_2, _1, _2)(i, j)==3);
  BOOST_CHECK(bind(sum_of_args_3, _1, _2, _3)(i, j, k)==6);
  BOOST_CHECK(bind(sum_of_args_4, _1, _2, _3, 4)(i, j, k)==10);
  BOOST_CHECK(bind(sum_of_args_5, _1, _2, _3, 4, 5)(i, j, k)==15);
  BOOST_CHECK(bind(sum_of_args_6, _1, _2, _3, 4, 5, 6)(i, j, k)==21);   
  BOOST_CHECK(bind(sum_of_args_7, _1, _2, _3, 4, 5, 6, 7)(i, j, k)==28);   
  BOOST_CHECK(bind(sum_of_args_8, _1, _2, _3, 4, 5, 6, 7, 8)(i, j, k)==36);   
  BOOST_CHECK(bind(sum_of_args_9, _1, _2, _3, 4, 5, 6, 7, 8, 9)(i, j, k)==45);
   
  // function compositions with bind
  BOOST_CHECK(bind(sum_of_args_3, bind(sum_of_args_2, _1, 2), 2, 3)(i)==8); 
  BOOST_CHECK(
    bind(sum_of_args_9,
       bind(sum_of_args_0),                             // 0
       bind(sum_of_args_1, _1),                         // 1
       bind(sum_of_args_2, _1, _2),                     // 3   
       bind(sum_of_args_3, _1, _2, _3),                 // 6 
       bind(sum_of_args_4, _1, _2, _3, 4),              // 10
       bind(sum_of_args_5, _1, _2, _3, 4, 5),           // 15
       bind(sum_of_args_6, _1, _2, _3, 4, 5, 6),        // 21
       bind(sum_of_args_7, _1, _2, _3, 4, 5, 6, 7),     // 28
       bind(sum_of_args_8, _1, _2, _3, 4, 5, 6, 7, 8)   // 36
    )(i, j, k) == 120);

  // deeper nesting
  result = 
    bind(sum_of_args_1,                        // 12
       bind(sum_of_args_4,                     // 12
            bind(sum_of_args_2,                // 3
                 bind(sum_of_args_1,           // 1
                      bind(sum_of_args_1, _1)  // 1
                      ), 
                 _2),
            _2,
            _3,
            4)
     )(i, j, k);
   BOOST_CHECK(result == 12);

  test_member_functions();


  return 0;
}
