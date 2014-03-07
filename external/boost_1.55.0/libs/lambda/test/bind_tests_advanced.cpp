//  bind_tests_advanced.cpp  -- The Boost Lambda Library ------------------
//
// Copyright (C) 2000-2003 Jaakko Jarvi (jaakko.jarvi@cs.utu.fi)
// Copyright (C) 2000-2003 Gary Powell (powellg@amazon.com)
// Copyright (C) 2010 Steven Watanabe
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


#include "boost/any.hpp"
#include "boost/type_traits/is_reference.hpp"
#include "boost/mpl/assert.hpp"
#include "boost/mpl/if.hpp"

#include <iostream>

#include <functional>

#include <algorithm>


using namespace boost::lambda;
namespace bl = boost::lambda;

int sum_0() { return 0; }
int sum_1(int a) { return a; }
int sum_2(int a, int b) { return a+b; }

int product_2(int a, int b) { return a*b; }

// unary function that returns a pointer to a binary function
typedef int (*fptr_type)(int, int);
fptr_type sum_or_product(bool x) { 
  return x ? sum_2 : product_2; 
}

// a nullary functor that returns a pointer to a unary function that
// returns a pointer to a binary function.
struct which_one {
  typedef fptr_type (*result_type)(bool x);
  template <class T> struct sig { typedef result_type type; };

  result_type operator()() const { return sum_or_product; }
};

void test_nested_binds()
{
  int j = 2; int k = 3;

// bind calls can be nested (the target function can be a lambda functor)
// The interpretation is, that the innermost lambda functor returns something
// that is bindable (another lambda functor, function pointer ...)
  bool condition;

  condition = true;
  BOOST_CHECK(bind(bind(&sum_or_product, _1), 1, 2)(condition)==3);
  BOOST_CHECK(bind(bind(&sum_or_product, _1), _2, _3)(condition, j, k)==5);

  condition = false;   
  BOOST_CHECK(bind(bind(&sum_or_product, _1), 1, 2)(condition)==2);
  BOOST_CHECK(bind(bind(&sum_or_product, _1), _2, _3)(condition, j, k)==6);


  which_one wo; 
  BOOST_CHECK(bind(bind(bind(wo), _1), _2, _3)(condition, j, k)==6);   


  return;
}


// unlambda -------------------------------------------------

  // Sometimes it may be necessary to prevent the argument substitution of
  // taking place. For example, we may end up with a nested bind expression 
  // inadvertently when using the target function is received as a parameter

template<class F>
int call_with_100(const F& f) {



  //  bind(f, _1)(make_const(100));
  // This would result in;
  // bind(_1 + 1, _1)(make_const(100)) , which would be a compile time error

  return bl::bind(unlambda(f), _1)(make_const(100));

  // for other functors than lambda functors, unlambda has no effect
  // (except for making them const)
}

template<class F>
int call_with_101(const F& f) {

  return bind(unlambda(f), _1)(make_const(101));

}


void test_unlambda() {

  int i = 1;

  BOOST_CHECK(unlambda(_1 + _2)(i, i) == 2);
  BOOST_CHECK(unlambda(++var(i))() == 2); 
  BOOST_CHECK(call_with_100(_1 + 1) == 101);


  BOOST_CHECK(call_with_101(_1 + 1) == 102);

  BOOST_CHECK(call_with_100(bl::bind(std_functor(std::bind1st(std::plus<int>(), 1)), _1)) == 101);

  // std_functor insturcts LL that the functor defines a result_type typedef
  // rather than a sig template.
  bl::bind(std_functor(std::plus<int>()), _1, _2)(i, i);
}




// protect ------------------------------------------------------------

// protect protects a lambda functor from argument substitution. 
// protect is useful e.g. with nested stl algorithm calls.

namespace ll {

struct for_each {
  
  // note, std::for_each returns it's last argument
  // We want the same behaviour from our ll::for_each.
  // However, the functor can be called with any arguments, and
  // the return type thus depends on the argument types.

  // 1. Provide a sig class member template:
 
  // The return type deduction system instantiate this class as:
  // sig<Args>::type, where Args is a boost::tuples::cons-list
  // The head type is the function object type itself 
  // cv-qualified (so it is possilbe to provide different return types
  // for differently cv-qualified operator()'s.

  // The tail type is the list of the types of the actual arguments the 
  // function was called with.
  // So sig should contain a typedef type, which defines a mapping from 
  // the operator() arguments to its return type.
  // Note, that it is possible to provide different sigs for the same functor
  // if the functor has several operator()'s, even if they have different 
  // number of arguments.

  // Note, that the argument types in Args are guaranteed to be non-reference
  // types, but they can have cv-qualifiers.

    template <class Args>
  struct sig { 
    typedef typename boost::remove_const<
          typename boost::tuples::element<3, Args>::type 
       >::type type; 
  };

  template <class A, class B, class C>
  C
  operator()(const A& a, const B& b, const C& c) const
  { return std::for_each(a, b, c);}
};

} // end of ll namespace

void test_protect() 
{
  int i = 0;
  int b[3][5];
  int* a[3];
  
  for(int j=0; j<3; ++j) a[j] = b[j];

  std::for_each(a, a+3, 
           bind(ll::for_each(), _1, _1 + 5, protect(_1 = ++var(i))));

  // This is how you could output the values (it is uncommented, no output
  // from a regression test file):
  //  std::for_each(a, a+3, 
  //                bind(ll::for_each(), _1, _1 + 5,
  //                     std::cout << constant("\nLine ") << (&_1 - a) << " : "
  //                     << protect(_1)
  //                     )
  //               );

  int sum = 0;
  
  std::for_each(a, a+3, 
           bind(ll::for_each(), _1, _1 + 5, 
                protect(sum += _1))
               );
  BOOST_CHECK(sum == (1+15)*15/2);

  sum = 0;

  std::for_each(a, a+3, 
           bind(ll::for_each(), _1, _1 + 5, 
                sum += 1 + protect(_1)) // add element count 
               );
  BOOST_CHECK(sum == (1+15)*15/2 + 15);

  (1 + protect(_1))(sum);

  int k = 0; 
  ((k += constant(1)) += protect(constant(2)))();
  BOOST_CHECK(k==1);

  k = 0; 
  ((k += constant(1)) += protect(constant(2)))()();
  BOOST_CHECK(k==3);

  // note, the following doesn't work:

  //  ((var(k) = constant(1)) = protect(constant(2)))();

  // (var(k) = constant(1))() returns int& and thus the
  // second assignment fails.

  // We should have something like:
  // bind(var, var(k) = constant(1)) = protect(constant(2)))();
  // But currently var is not bindable.

  // The same goes with ret. A bindable ret could be handy sometimes as well
  // (protect(std::cout << _1), std::cout << _1)(i)(j); does not work
  // because the comma operator tries to store the result of the evaluation
  // of std::cout << _1 as a copy (and you can't copy std::ostream).
  // something like this:
  // (protect(std::cout << _1), bind(ref, std::cout << _1))(i)(j); 


  // the stuff below works, but we do not want extra output to 
  // cout, must be changed to stringstreams but stringstreams do not
  // work due to a bug in the type deduction. Will be fixed...
#if 0
  // But for now, ref is not bindable. There are other ways around this:

    int x = 1, y = 2;
    (protect(std::cout << _1), (std::cout << _1, 0))(x)(y);

  // added one dummy value to make the argument to comma an int 
  // instead of ostream& 

  // Note, the same problem is more apparent without protect
  //   (std::cout << 1, std::cout << constant(2))(); // does not work

    (boost::ref(std::cout << 1), std::cout << constant(2))(); // this does

#endif

}


void test_lambda_functors_as_arguments_to_lambda_functors() {

// lambda functor is a function object, and can therefore be used
// as an argument to another lambda functors function call object.

  // Note however, that the argument/type substitution is not entered again.
  // This means, that something like this will not work:

    (_1 + _2)(_1, make_const(7));
    (_1 + _2)(bind(&sum_0), make_const(7)); 

    // or it does work, but the effect is not to call
    // sum_0() + 7, but rather
    // bind(sum_0) + 7, which results in another lambda functor
    // (lambda functor + int) and can be called again
  BOOST_CHECK((_1 + _2)(bind(&sum_0), make_const(7))() == 7); 
   
  int i = 3, j = 12; 
  BOOST_CHECK((_1 - _2)(_2, _1)(i, j) == j - i);

  // also, note that lambda functor are no special case for bind if received
  // as a parameter. In oder to be bindable, the functor must
  // defint the sig template, or then
  // the return type must be defined within the bind call. Lambda functors
  // do define the sig template, so if the return type deduction system
  // covers the case, there is no need to specify the return type 
  // explicitly.

  int a = 5, b = 6;

  // Let type deduction find out the return type
  BOOST_CHECK(bind(_1, _2, _3)(unlambda(_1 + _2), a, b) == 11);

  //specify it yourself:
  BOOST_CHECK(bind(_1, _2, _3)(ret<int>(_1 + _2), a, b) == 11);
  BOOST_CHECK(ret<int>(bind(_1, _2, _3))(_1 + _2, a, b) == 11);
  BOOST_CHECK(bind<int>(_1, _2, _3)(_1 + _2, a, b) == 11);

  bind(_1,1.0)(_1+_1);
  return; 

}


void test_const_parameters() {

  //  (_1 + _2)(1, 2); // this would fail, 

  // Either make arguments const:
  BOOST_CHECK((_1 + _2)(make_const(1), make_const(2)) == 3); 

  // Or use const_parameters:
  BOOST_CHECK(const_parameters(_1 + _2)(1, 2) == 3);



}

void test_rvalue_arguments()
{
  // Not quite working yet.
  // Problems with visual 7.1
  // BOOST_CHECK((_1 + _2)(1, 2) == 3);
}

void test_break_const() 
{

  // break_const is currently unnecessary, as LL supports perfect forwarding
  // for up to there argument lambda functors, and LL does not support
  // lambda functors with more than 3 args.

  // I'll keep the test case around anyway, if more arguments will be supported
  // in the future. 


  
  // break_const breaks constness! Be careful!
  // You need this only if you need to have side effects on some argument(s)
  // and some arguments are non-const rvalues and your lambda functors
  // take more than 3 arguments. 

  
  int i = 1;
  //  OLD COMMENT: (_1 += _2)(i, 2) // fails, 2 is a non-const rvalue  
  //  OLD COMMENT:  const_parameters(_1 += _2)(i, 2) // fails, side-effect to i
  break_const(_1 += _2)(i, 2); // ok
  BOOST_CHECK(i == 3);
}

template<class T>
struct func {
  template<class Args>
  struct sig {
    typedef typename boost::tuples::element<1, Args>::type arg1;
    // If the argument type is not the same as the expected type,
    // return void, which will cause an error.  Note that we
    // can't just assert that the types are the same, because
    // both const and non-const versions can be instantiated
    // even though only one is ultimately used.
    typedef typename boost::mpl::if_<boost::is_same<arg1, T>,
      typename boost::remove_const<arg1>::type,
      void
    >::type type;
  };
  template<class U>
  U operator()(const U& arg) const {
    return arg;
  }
};

void test_sig()
{
  int i = 1;
  BOOST_CHECK(bind(func<int>(), 1)() == 1);
  BOOST_CHECK(bind(func<const int>(), _1)(static_cast<const int&>(i)) == 1);
  BOOST_CHECK(bind(func<int>(), _1)(i) == 1);
}

class base {
public:
  virtual int foo() = 0;
};

class derived : public base {
public:
  virtual int foo() {
    return 1;
  }
};

void test_abstract()
{
  derived d;
  base& b = d;
  BOOST_CHECK(bind(&base::foo, var(b))() == 1);
  BOOST_CHECK(bind(&base::foo, *_1)(&b) == 1);
}

int test_main(int, char *[]) {

  test_nested_binds();
  test_unlambda();
  test_protect();
  test_lambda_functors_as_arguments_to_lambda_functors();
  test_const_parameters();
  test_rvalue_arguments(); 
  test_break_const(); 
  test_sig();
  test_abstract();
  return 0;
}
