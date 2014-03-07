//  Copyright (C) 2010 Edward Diener
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_CXX11_VARIADIC_MACROS
//  TITLE:         C++0x variadic macros unavailable
//  DESCRIPTION:   The compiler does not support C++0x variadic macros

// This is a simple test

#define TEST_VARIADIC_MACRO_SIMPLE(avalue,...) __VA_ARGS__

/* 

  This is a more complicated test, which Steve Watanabe graciously 
  supplied, when I asked if it were possible to strip the parantheses 
  from a macro argument. I have changed the names somewhat to prevent 
  any common clashes with other macros in the config testing suite 
  by prepending to each macro name TEST_VARIADIC_MACRO_.
  
  You may find this test overdone and may want to remove it.
  
*/

#define TEST_VARIADIC_MACRO_CAT(x, y) TEST_VARIADIC_MACRO_CAT_I(x, y)
#define TEST_VARIADIC_MACRO_CAT_I(x, y) x ## y

#define TEST_VARIADIC_MACRO_APPLY(macro, args) TEST_VARIADIC_MACRO_APPLY_I(macro, args)
#define TEST_VARIADIC_MACRO_APPLY_I(macro, args) macro args

#define TEST_VARIADIC_MACRO_STRIP_PARENS(x) TEST_VARIADIC_MACRO_EVAL((TEST_VARIADIC_MACRO_STRIP_PARENS_I x), x)
#define TEST_VARIADIC_MACRO_STRIP_PARENS_I(...) 1,1

#define TEST_VARIADIC_MACRO_EVAL(test, x) TEST_VARIADIC_MACRO_EVAL_I(test, x)
#define TEST_VARIADIC_MACRO_EVAL_I(test, x) TEST_VARIADIC_MACRO_MAYBE_STRIP_PARENS(TEST_VARIADIC_MACRO_TEST_ARITY test, x)

#define TEST_VARIADIC_MACRO_TEST_ARITY(...) TEST_VARIADIC_MACRO_APPLY(TEST_VARIADIC_MACRO_TEST_ARITY_I, (__VA_ARGS__, 2, 1))
#define TEST_VARIADIC_MACRO_TEST_ARITY_I(a,b,c,...) c

#define TEST_VARIADIC_MACRO_MAYBE_STRIP_PARENS(cond, x) TEST_VARIADIC_MACRO_MAYBE_STRIP_PARENS_I(cond, x)
#define TEST_VARIADIC_MACRO_MAYBE_STRIP_PARENS_I(cond, x) TEST_VARIADIC_MACRO_CAT(TEST_VARIADIC_MACRO_MAYBE_STRIP_PARENS_, cond)(x)

#define TEST_VARIADIC_MACRO_MAYBE_STRIP_PARENS_1(x) x
#define TEST_VARIADIC_MACRO_MAYBE_STRIP_PARENS_2(x) TEST_VARIADIC_MACRO_APPLY(TEST_VARIADIC_MACRO_MAYBE_STRIP_PARENS_2_I, x)
#define TEST_VARIADIC_MACRO_MAYBE_STRIP_PARENS_2_I(...) __VA_ARGS__

namespace boost_no_cxx11_variadic_macros {

void quiet_warning(int){}

template<TEST_VARIADIC_MACRO_STRIP_PARENS((typename T,int))> struct test_variadic_macro_class {};

int test()
{

  int x = TEST_VARIADIC_MACRO_STRIP_PARENS(3);
  quiet_warning(x);
  return 0;
}

}
