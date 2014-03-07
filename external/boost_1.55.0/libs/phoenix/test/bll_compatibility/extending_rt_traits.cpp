//  extending_return_type_traits.cpp  -- The Boost Lambda Library --------
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
#include "boost/lambda/lambda.hpp"
#include "boost/lambda/detail/suppress_unused.hpp"

#include <iostream>

#include <functional>

#include <algorithm>

using boost::lambda::detail::suppress_unused_variable_warnings;

class A {};
class B {};

using namespace boost::lambda; 


B operator--(const A&, int) { return B(); }
B operator--(A&) { return B(); }
B operator++(const A&, int) { return B(); }
B operator++(A&) { return B(); }
B operator-(const A&) { return B(); }
B operator+(const A&) { return B(); }

B operator!(const A&) { return B(); }

B operator&(const A&) { return B(); }
B operator*(const A&) { return B(); }

namespace boost {
namespace lambda {

  // unary + and -
template<class Act>
struct plain_return_type_1<unary_arithmetic_action<Act>, A > {
  typedef B type;
};

  // post incr/decr
template<class Act>
struct plain_return_type_1<post_increment_decrement_action<Act>, A > {
  typedef B type;
};

  // pre incr/decr
template<class Act>
struct plain_return_type_1<pre_increment_decrement_action<Act>, A > {
  typedef B type;
};
  // !
template<> 
struct plain_return_type_1<logical_action<not_action>, A> {
  typedef B type;
};
  // &
template<> 
struct plain_return_type_1<other_action<addressof_action>, A> {
  typedef B type;
};
  // *
template<> 
struct plain_return_type_1<other_action<contentsof_action>, A> {
  typedef B type;
};


} // lambda
} // boost

void ok(B /*b*/) {}

void test_unary_operators() 
{
  A a; int i = 1;
  ok((++_1)(a));
  ok((--_1)(a));
  ok((_1++)(a));
  ok((_1--)(a));
  ok((+_1)(a));
  ok((-_1)(a));
  ok((!_1)(a));
  ok((&_1)(a));
  ok((*_1)(a));

  BOOST_CHECK((*_1)(make_const(&i)) == 1);
}

class X {};
class Y {};
class Z {};

Z operator+(const X&, const Y&) { return Z(); }
Z operator-(const X&, const Y&) { return Z(); }
X operator*(const X&, const Y&) { return X(); }

Z operator/(const X&, const Y&) { return Z(); }
Z operator%(const X&, const Y&) { return Z(); }

class XX {};
class YY {};
class ZZ {};
class VV {};

// it is possible to support differently cv-qualified versions
YY operator*(XX&, YY&) { return YY(); }
ZZ operator*(const XX&, const YY&) { return ZZ(); }
XX operator*(volatile XX&, volatile YY&) { return XX(); }
VV operator*(const volatile XX&, const volatile YY&) { return VV(); }

// the traits can be more complex:
template <class T>
class my_vector {};

template<class A, class B> 
my_vector<typename return_type_2<arithmetic_action<plus_action>, A&, B&>::type>
operator+(const my_vector<A>& /*a*/, const my_vector<B>& /*b*/)
{
  typedef typename 
    return_type_2<arithmetic_action<plus_action>, A&, B&>::type res_type;
  return my_vector<res_type>();
}



// bitwise ops:
X operator<<(const X&, const Y&) { return X(); }
Z operator>>(const X&, const Y&) { return Z(); }
Z operator&(const X&, const Y&) { return Z(); }
Z operator|(const X&, const Y&) { return Z(); }
Z operator^(const X&, const Y&) { return Z(); }

// comparison ops:

X operator<(const X&, const Y&) { return X(); }
Z operator>(const X&, const Y&) { return Z(); }
Z operator<=(const X&, const Y&) { return Z(); }
Z operator>=(const X&, const Y&) { return Z(); }
Z operator==(const X&, const Y&) { return Z(); }
Z operator!=(const X&, const Y&) { return Z(); }

// logical 

X operator&&(const X&, const Y&) { return X(); }
Z operator||(const X&, const Y&) { return Z(); }

// arithh assignment

Z operator+=( X&, const Y&) { return Z(); }
Z operator-=( X&, const Y&) { return Z(); }
Y operator*=( X&, const Y&) { return Y(); }
Z operator/=( X&, const Y&) { return Z(); }
Z operator%=( X&, const Y&) { return Z(); }

// bitwise assignment
Z operator<<=( X&, const Y&) { return Z(); }
Z operator>>=( X&, const Y&) { return Z(); }
Y operator&=( X&, const Y&) { return Y(); }
Z operator|=( X&, const Y&) { return Z(); }
Z operator^=( X&, const Y&) { return Z(); }

// assignment
class Assign {
public:
  void operator=(const Assign& /*a*/) {}
  X operator[](const int& /*i*/) { return X(); }
};



namespace boost {
namespace lambda {
  
  // you can do action groups
template<class Act> 
struct plain_return_type_2<arithmetic_action<Act>, X, Y> {
  typedef Z type;
};

  // or specialize the exact action
template<> 
struct plain_return_type_2<arithmetic_action<multiply_action>, X, Y> {
  typedef X type;
};

  // if you want to make a distinction between differently cv-qualified
  // types, you need to specialize on a different level:
template<> 
struct return_type_2<arithmetic_action<multiply_action>, XX, YY> {
  typedef YY type;
};
template<> 
struct return_type_2<arithmetic_action<multiply_action>, const XX, const YY> {
  typedef ZZ type;
};
template<> 
struct return_type_2<arithmetic_action<multiply_action>, volatile XX, volatile YY> {
  typedef XX type;
};
template<> 
struct return_type_2<arithmetic_action<multiply_action>, volatile const XX, const volatile YY> {
  typedef VV type;
};

  // the mapping can be more complex:
template<class A, class B> 
struct plain_return_type_2<arithmetic_action<plus_action>, my_vector<A>, my_vector<B> > {
  typedef typename 
    return_type_2<arithmetic_action<plus_action>, A&, B&>::type res_type;
  typedef my_vector<res_type> type;
};

  // bitwise binary:
  // you can do action groups
template<class Act> 
struct plain_return_type_2<bitwise_action<Act>, X, Y> {
  typedef Z type;
};

  // or specialize the exact action
template<> 
struct plain_return_type_2<bitwise_action<leftshift_action>, X, Y> {
  typedef X type;
};

  // comparison binary:
  // you can do action groups
template<class Act> 
struct plain_return_type_2<relational_action<Act>, X, Y> {
  typedef Z type;
};

  // or specialize the exact action
template<> 
struct plain_return_type_2<relational_action<less_action>, X, Y> {
  typedef X type;
};

  // logical binary:
  // you can do action groups
template<class Act> 
struct plain_return_type_2<logical_action<Act>, X, Y> {
  typedef Z type;
};

  // or specialize the exact action
template<> 
struct plain_return_type_2<logical_action<and_action>, X, Y> {
  typedef X type;
};

  // arithmetic assignment :
  // you can do action groups
template<class Act> 
struct plain_return_type_2<arithmetic_assignment_action<Act>, X, Y> {
  typedef Z type;
};

  // or specialize the exact action
template<> 
struct plain_return_type_2<arithmetic_assignment_action<multiply_action>, X, Y> {
  typedef Y type;
};

  // arithmetic assignment :
  // you can do action groups
template<class Act> 
struct plain_return_type_2<bitwise_assignment_action<Act>, X, Y> {
  typedef Z type;
};

  // or specialize the exact action
template<> 
struct plain_return_type_2<bitwise_assignment_action<and_action>, X, Y> {
  typedef Y type;
};

  // assignment
template<> 
struct plain_return_type_2<other_action<assignment_action>, Assign, Assign> {
  typedef void type;
};
  // subscript
template<> 
struct plain_return_type_2<other_action<subscript_action>, Assign, int> {
  typedef X type;
};


} // end lambda
} // end boost



void test_binary_operators() {

  X x; Y y;
  (_1 + _2)(x, y);
  (_1 - _2)(x, y);
  (_1 * _2)(x, y);
  (_1 / _2)(x, y);
  (_1 % _2)(x, y);


  // make a distinction between differently cv-qualified operators
  XX xx; YY yy;
  const XX& cxx = xx;
  const YY& cyy = yy;
  volatile XX& vxx = xx;
  volatile YY& vyy = yy;
  const volatile XX& cvxx = xx;
  const volatile YY& cvyy = yy;

  ZZ dummy1 = (_1 * _2)(cxx, cyy);
  YY dummy2 = (_1 * _2)(xx, yy);
  XX dummy3 = (_1 * _2)(vxx, vyy);
  VV dummy4 = (_1 * _2)(cvxx, cvyy);

  suppress_unused_variable_warnings(dummy1);
  suppress_unused_variable_warnings(dummy2);
  suppress_unused_variable_warnings(dummy3);
  suppress_unused_variable_warnings(dummy4);

  my_vector<int> v1; my_vector<double> v2;
  my_vector<double> d = (_1 + _2)(v1, v2);

  suppress_unused_variable_warnings(d);

  // bitwise

  (_1 << _2)(x, y);
  (_1 >> _2)(x, y);
  (_1 | _2)(x, y);
  (_1 & _2)(x, y);
  (_1 ^ _2)(x, y);

  // comparison
  
  (_1 < _2)(x, y);
  (_1 > _2)(x, y);
  (_1 <= _2)(x, y);
  (_1 >= _2)(x, y);
  (_1 == _2)(x, y);
  (_1 != _2)(x, y);

  // logical
 
  (_1 || _2)(x, y);
  (_1 && _2)(x, y);

  // arithmetic assignment
  (_1 += _2)(x, y);
  (_1 -= _2)(x, y);
  (_1 *= _2)(x, y);
  (_1 /= _2)(x, y);
  (_1 %= _2)(x, y);
 
  // bitwise assignment
  (_1 <<= _2)(x, y);
  (_1 >>= _2)(x, y);
  (_1 |= _2)(x, y);
  (_1 &= _2)(x, y);
  (_1 ^= _2)(x, y);

}


int test_main(int, char *[]) {
  test_unary_operators();
  test_binary_operators();
  return 0;
}






