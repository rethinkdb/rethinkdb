// -- control_structures.cpp -- The Boost Lambda Library ------------------
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
#include "boost/lambda/if.hpp"
#include "boost/lambda/loops.hpp"

#include <iostream>
#include <algorithm>
#include <vector>

using namespace boost;

using boost::lambda::constant;
using boost::lambda::_1;
using boost::lambda::_2;
using boost::lambda::_3;
using boost::lambda::make_const;
using boost::lambda::for_loop;
using boost::lambda::while_loop;
using boost::lambda::do_while_loop;
using boost::lambda::if_then;
using boost::lambda::if_then_else;
using boost::lambda::if_then_else_return;

// 2 container for_each
template <class InputIter1, class InputIter2, class Function>
Function for_each(InputIter1 first, InputIter1 last, 
                  InputIter2 first2, Function f) {
  for ( ; first != last; ++first, ++first2)
    f(*first, *first2);
  return f;
}

void simple_loops() {

  // for loops ---------------------------------------------------------
  int i;
  int arithmetic_series = 0;
  for_loop(_1 = 0, _1 < 10, _1++, arithmetic_series += _1)(i);
  BOOST_CHECK(arithmetic_series == 45);

  // no body case
  for_loop(boost::lambda::var(i) = 0, boost::lambda::var(i) < 100, ++boost::lambda::var(i))();
  BOOST_CHECK(i == 100);
 
  // while loops -------------------------------------------------------
  int a = 0, b = 0, c = 0;

  while_loop((_1 + _2) >= (_1 * _2), (++_1, ++_2, ++_3))(a, b, c); 
  BOOST_CHECK(c == 3);

  int count;
  count = 0; i = 0;
  while_loop(_1++ < 10, ++boost::lambda::var(count))(i);
  BOOST_CHECK(count == 10);

  // note that the first parameter of do_while_loop is the condition
  count = 0; i = 0;
  do_while_loop(_1++ < 10, ++boost::lambda::var(count))(i);
  BOOST_CHECK(count == 11);

  a = 0;
  do_while_loop(constant(false), _1++)(a);
  BOOST_CHECK(a == 1);
 
  // no body cases
  a = 40; b = 30;
  while_loop(--_1 > _2)(a, b);
  BOOST_CHECK(a == b);

  // (the no body case for do_while_loop is pretty redundant)
  a = 40; b = 30;
  do_while_loop(--_1 > _2)(a, b);
  BOOST_CHECK(a == b);


}

void simple_ifs () {

  int value = 42;
  if_then(_1 < 0, _1 = 0)(value);
  BOOST_CHECK(value == 42);

  value = -42;
  if_then(_1 < 0, _1 = -_1)(value);
  BOOST_CHECK(value == 42);

  int min;
  if_then_else(_1 < _2, boost::lambda::var(min) = _1, boost::lambda::var(min) = _2)
     (make_const(1), make_const(2));
  BOOST_CHECK(min == 1);

  if_then_else(_1 < _2, boost::lambda::var(min) = _1, boost::lambda::var(min) = _2)
     (make_const(5), make_const(3));
  BOOST_CHECK(min == 3);

  int x, y;
  x = -1; y = 1;
  BOOST_CHECK(if_then_else_return(_1 < _2, _2, _1)(x, y) == (std::max)(x ,y));
  BOOST_CHECK(if_then_else_return(_1 < _2, _2, _1)(y, x) == (std::max)(x ,y));
}


int test_main(int, char *[]) 
{
  simple_loops();
  simple_ifs();
  return 0;
}
