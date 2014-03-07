//  cast_tests.cpp  -- The Boost Lambda Library ------------------
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

#include "boost/lambda/casts.hpp"

#include <string>

using namespace boost::lambda;
using namespace std;

class base {
  int x;
public:
  virtual std::string class_name() const { return "const base"; }
  virtual std::string class_name() { return "base"; }
  virtual ~base() {}
};

class derived : public base {
  int y[100];
public:
  virtual std::string class_name() const { return "const derived"; }
  virtual std::string class_name() { return "derived"; }
};




void do_test() {

  derived *p_derived = new derived;
  base *p_base = new base;

  base *b = 0;
  derived *d = 0;

  (var(b) = ll_static_cast<base *>(p_derived))();
  (var(d) = ll_static_cast<derived *>(b))();
  
  BOOST_CHECK(b->class_name() == "derived");
  BOOST_CHECK(d->class_name() == "derived");

  (var(b) = ll_dynamic_cast<derived *>(b))();
  BOOST_CHECK(b != 0);
  BOOST_CHECK(b->class_name() == "derived");

  (var(d) = ll_dynamic_cast<derived *>(p_base))();
  BOOST_CHECK(d == 0);

  

  const derived* p_const_derived = p_derived;

  BOOST_CHECK(p_const_derived->class_name() == "const derived");
  (var(d) = ll_const_cast<derived *>(p_const_derived))();
  BOOST_CHECK(d->class_name() == "derived");

  int i = 10;
  char* cp = reinterpret_cast<char*>(&i);

  int* ip;
  (var(ip) = ll_reinterpret_cast<int *>(cp))();
  BOOST_CHECK(*ip == 10);


  // typeid 

  BOOST_CHECK(string(ll_typeid(d)().name()) == string(typeid(d).name()));

 
  // sizeof

  BOOST_CHECK(ll_sizeof(_1)(p_derived) == sizeof(p_derived));
  BOOST_CHECK(ll_sizeof(_1)(*p_derived) == sizeof(*p_derived));
  BOOST_CHECK(ll_sizeof(_1)(p_base) == sizeof(p_base));
  BOOST_CHECK(ll_sizeof(_1)(*p_base) == sizeof(*p_base));

  int an_array[100];
  BOOST_CHECK(ll_sizeof(_1)(an_array) == 100 * sizeof(int));

  delete p_derived;
  delete p_base;


}

int test_main(int, char *[]) {

  do_test();
  return 0;
}
