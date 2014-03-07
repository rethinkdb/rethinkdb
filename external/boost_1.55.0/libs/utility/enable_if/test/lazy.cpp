// Boost enable_if library

// Copyright 2003 (c) The Trustees of Indiana University.

// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//    Authors: Jaakko Jarvi (jajarvi at osl.iu.edu)
//             Jeremiah Willcock (jewillco at osl.iu.edu)
//             Andrew Lumsdaine (lums at osl.iu.edu)

#include <boost/test/minimal.hpp>

#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>

using boost::enable_if_c;
using boost::lazy_enable_if_c;

// This class provides a reduced example of a traits class for
// computing the result of multiplying two types.  The member typedef
// 'type' in this traits class defines the return type of this
// operator.  The return type member is invalid unless both arguments
// for mult_traits are values that mult_traits expects (ints in this
// case).  This kind of situation may arise if a traits class only
// makes sense for some set of types, not all C++ types.

template <class T> struct is_int {
  BOOST_STATIC_CONSTANT(bool, value = (boost::is_same<T, int>::value));
};

template <class T, class U>
struct mult_traits {
  typedef typename T::does_not_exist type;
};

template <>
struct mult_traits<int, int> {
  typedef int type;
};


// Next, a forwarding function mult() is defined.  It is enabled only
// when both arguments are of type int.  The first version, using
// non-lazy enable_if_c does not work.

#if 0
template <class T, class U>
typename enable_if_c<
  is_int<T>::value && is_int<U>::value,
  typename mult_traits<T, U>::type
>::type
mult(const T& x, const U& y) {return x * y;}
#endif

// A correct version uses lazy_enable_if_c.
// This template removes compiler errors from invalid code used as an
// argument to enable_if_c.

#if 1
template <class T, class U>
typename lazy_enable_if_c<
  is_int<T>::value & is_int<U>::value,
  mult_traits<T, U> 
>::type
mult(const T& x, const U& y) {return x * y;}
#endif

double mult(int i, double d) { return (double)i * d; }

int test_main(int, char*[])
{


  BOOST_CHECK(mult(1, 2) == 2);

  BOOST_CHECK(mult(1, 3.0) == 3.0);

  return 0;
}

