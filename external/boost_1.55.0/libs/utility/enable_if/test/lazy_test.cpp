// Boost enable_if library

// Copyright 2003 (c) The Trustees of Indiana University.

// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//    Authors: Jaakko Jarvi (jajarvi at osl.iu.edu)
//             Jeremiah Willcock (jewillco at osl.iu.edu)
//             Andrew Lumsdaine (lums at osl.iu.edu)

// Testing all variations of lazy_enable_if.

#include <boost/test/minimal.hpp>
#include <boost/mpl/not.hpp>

#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>

using boost::lazy_enable_if;
using boost::lazy_disable_if;

using boost::lazy_enable_if_c;
using boost::lazy_disable_if_c;


template <class T>
struct is_int_or_double {
  BOOST_STATIC_CONSTANT(bool, 
    value = (boost::is_same<T, int>::value || 
             boost::is_same<T, double>::value));
};

template <class T>
struct some_traits {
  typedef typename T::does_not_exist type;
};

template <>
struct some_traits<int> {
  typedef bool type;
};

template <>
struct some_traits<double> {
  typedef bool type;
};

template <class T>
struct make_bool {
  typedef bool type;
};

template <>
struct make_bool<int> {};

template <>
struct make_bool<double> {};

namespace A {

  template<class T>
  typename lazy_enable_if<is_int_or_double<T>, some_traits<T> >::type
  foo(T t) { return true; }

  template<class T>
  typename lazy_enable_if_c<is_int_or_double<T>::value, some_traits<T> >::type
  foo2(T t) { return true; }
}

namespace B {
  template<class T>
  typename lazy_disable_if<is_int_or_double<T>, make_bool<T> >::type
  foo(T t) { return false; }

  template<class T>
  typename lazy_disable_if_c<is_int_or_double<T>::value, make_bool<T> >::type
  foo2(T t) { return false; }
}

int test_main(int, char*[])
{
  using namespace A;
  using namespace B;
  BOOST_CHECK(foo(1));
  BOOST_CHECK(foo(1.0));

  BOOST_CHECK(!foo("1"));  
  BOOST_CHECK(!foo(static_cast<void*>(0)));  

  BOOST_CHECK(foo2(1));
  BOOST_CHECK(foo2(1.0));

  BOOST_CHECK(!foo2("1"));  
  BOOST_CHECK(!foo2(static_cast<void*>(0)));  

  return 0;
}

