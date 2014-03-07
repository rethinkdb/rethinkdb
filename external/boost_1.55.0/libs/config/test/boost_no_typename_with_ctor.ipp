//  Copyright (C) 2008 N. Musatti
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_TYPENAME_WITH_CTOR
//  TITLE:         Use of typename keyword with constructors
//  DESCRIPTION:   If the compiler rejects the typename keyword when calling
//                 the constructor of a dependent type

namespace boost_no_typename_with_ctor {

struct A {};

template <typename T>
struct B {
  typedef T type;
};

template <typename T>
typename T::type f() {
  return typename T::type();
}

int test() {
  A a = f<B<A> >();
  (void)a;
  return 0;
}

}

