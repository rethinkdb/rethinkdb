//  member_pointer_test.cpp  -- The Boost Lambda Library ------------------
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


#include "boost/lambda/lambda.hpp"
#include "boost/lambda/bind.hpp"

#include <string>

using namespace boost::lambda;
using namespace std;


struct my_struct {
my_struct(int x) : mem(x) {};

  int mem;

  int fooc() const { return mem; }
  int foo() { return mem; }
  int foo1c(int y) const { return y + mem; }
  int foo1(int y)  { return y + mem; }
  int foo2c(int y, int x) const { return y + x + mem; }
  int foo2(int y, int x) { return y + x + mem; }
  int foo3c(int y, int x, int z) const { return y + x + z + mem; }
  int foo3(int y, int x, int z ){ return y + x + z + mem; }
  int foo4c(int a1, int a2, int a3, int a4) const { return a1+a2+a3+a4+mem; }
  int foo4(int a1, int a2, int a3, int a4){ return a1+a2+a3+a4+mem; }

  int foo3default(int y = 1, int x = 2, int z = 3) { return y + x + z + mem; }
};

my_struct x(3);

void pointer_to_data_member_tests() {

  //  int i = 0;
  my_struct *y = &x;

  BOOST_CHECK((_1 ->* &my_struct::mem)(y) == 3);

  (_1 ->* &my_struct::mem)(y) = 4;
  BOOST_CHECK(x.mem == 4);

  ((_1 ->* &my_struct::mem) = 5)(y);
  BOOST_CHECK(x.mem == 5);

  // &my_struct::mem is a temporary, must be constified
  ((y ->* _1) = 6)(make_const(&my_struct::mem));
  BOOST_CHECK(x.mem == 6);

  ((_1 ->* _2) = 7)(y, make_const(&my_struct::mem));
  BOOST_CHECK(x.mem == 7);

}

void pointer_to_member_function_tests() {  

  my_struct *y = new my_struct(1);
  BOOST_CHECK( (_1 ->* &my_struct::foo)(y)() == (y->mem));
  BOOST_CHECK( (_1 ->* &my_struct::fooc)(y)() == (y->mem));
  BOOST_CHECK( (y ->* _1)(make_const(&my_struct::foo))() == (y->mem));
  BOOST_CHECK( (y ->* _1)(make_const(&my_struct::fooc))() == (y->mem));
  BOOST_CHECK( (_1 ->* _2)(y, make_const(&my_struct::foo))() == (y->mem));
  BOOST_CHECK( (_1 ->* _2)(y, make_const(&my_struct::fooc))() == (y->mem));

  BOOST_CHECK( (_1 ->* &my_struct::foo1)(y)(1) == (y->mem+1));
  BOOST_CHECK( (_1 ->* &my_struct::foo1c)(y)(1) == (y->mem+1));
  BOOST_CHECK( (y ->* _1)(make_const(&my_struct::foo1))(1) == (y->mem+1));
  BOOST_CHECK( (y ->* _1)(make_const(&my_struct::foo1c))(1) == (y->mem+1));
  BOOST_CHECK( (_1 ->* _2)(y, make_const(&my_struct::foo1))(1) == (y->mem+1));
  BOOST_CHECK( (_1 ->* _2)(y, make_const(&my_struct::foo1c))(1) == (y->mem+1));

  BOOST_CHECK( (_1 ->* &my_struct::foo2)(y)(1,2) == (y->mem+1+2));
  BOOST_CHECK( (_1 ->* &my_struct::foo2c)(y)(1,2) == (y->mem+1+2));
  BOOST_CHECK( (y ->* _1)(make_const(&my_struct::foo2))(1,2) == (y->mem+1+2));
  BOOST_CHECK( (y ->* _1)(make_const(&my_struct::foo2c))(1,2) == (y->mem+1+2));
  BOOST_CHECK( (_1 ->* _2)(y, make_const(&my_struct::foo2))(1,2) == (y->mem+1+2));
  BOOST_CHECK( (_1 ->* _2)(y, make_const(&my_struct::foo2c))(1,2) == (y->mem+1+2));

  BOOST_CHECK( (_1 ->* &my_struct::foo3)(y)(1,2,3) == (y->mem+1+2+3));
  BOOST_CHECK( (_1 ->* &my_struct::foo3c)(y)(1,2,3) == (y->mem+1+2+3));
  BOOST_CHECK( (y ->* _1)(make_const(&my_struct::foo3))(1,2,3) == (y->mem+1+2+3));
  BOOST_CHECK( (y ->* _1)(make_const(&my_struct::foo3c))(1,2,3) == (y->mem+1+2+3));
  BOOST_CHECK( (_1 ->* _2)(y, make_const(&my_struct::foo3))(1,2,3) == (y->mem+1+2+3));
  BOOST_CHECK( (_1 ->* _2)(y, make_const(&my_struct::foo3c))(1,2,3) == (y->mem+1+2+3));

  BOOST_CHECK( (_1 ->* &my_struct::foo4)(y)(1,2,3,4) == (y->mem+1+2+3+4));
  BOOST_CHECK( (_1 ->* &my_struct::foo4c)(y)(1,2,3,4) == (y->mem+1+2+3+4));
  BOOST_CHECK( (y ->* _1)(make_const(&my_struct::foo4))(1,2,3,4) == (y->mem+1+2+3+4));
  BOOST_CHECK( (y ->* _1)(make_const(&my_struct::foo4c))(1,2,3,4) == (y->mem+1+2+3+4));
  BOOST_CHECK( (_1 ->* _2)(y, make_const(&my_struct::foo4))(1,2,3,4) == (y->mem+1+2+3+4));
  BOOST_CHECK( (_1 ->* _2)(y, make_const(&my_struct::foo4c))(1,2,3,4) == (y->mem+1+2+3+4));



  // member functions with default values do not work (inherent language issue)
  //  BOOST_CHECK( (_1 ->* &my_struct::foo3default)(y)() == (y->mem+1+2+3));

}

class A {};
class B {};
class C {};
class D {};

// ->* can be overloaded to do anything
bool operator->*(A /*a*/, B /*b*/) {
  return false; 
}

bool operator->*(B /*b*/, A /*a*/) {
  return true; 
}

// let's provide specializations to take care of the return type deduction.
// Note, that you need to provide all four cases for non-const and const
// or use the plain_return_type_2 template.
namespace boost {
namespace lambda {

template <>
struct return_type_2<other_action<member_pointer_action>, B, A> {
  typedef bool type;
};

template<>
struct return_type_2<other_action<member_pointer_action>, const B, A> {
  typedef bool type;
};

template<>
struct return_type_2<other_action<member_pointer_action>, B, const A> {
  typedef bool type;
};

template<>
struct return_type_2<other_action<member_pointer_action>, const B, const A> {
  typedef bool type;
};




} // lambda
} // boost

void test_overloaded_pointer_to_member()
{
  A a; B b;

  // this won't work, can't deduce the return type
  //  BOOST_CHECK((_1->*_2)(a, b) == false); 

  // ret<bool> gives the return type
  BOOST_CHECK(ret<bool>(_1->*_2)(a, b) == false);
  BOOST_CHECK(ret<bool>(a->*_1)(b) == false);
  BOOST_CHECK(ret<bool>(_1->*b)(a) == false);
  BOOST_CHECK((ret<bool>((var(a))->*b))() == false);
  BOOST_CHECK((ret<bool>((var(a))->*var(b)))() == false);


  // this is ok without ret<bool> due to the return_type_2 spcialization above
  BOOST_CHECK((_1->*_2)(b, a) == true);
  BOOST_CHECK((b->*_1)(a) == true);
  BOOST_CHECK((_1->*a)(b) == true);
  BOOST_CHECK((var(b)->*a)() == true);
  return;
}


int test_main(int, char *[]) {

  pointer_to_data_member_tests();
  pointer_to_member_function_tests();
  test_overloaded_pointer_to_member();
  return 0;
}

