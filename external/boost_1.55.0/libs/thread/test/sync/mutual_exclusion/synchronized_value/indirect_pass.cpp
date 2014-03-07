// Copyright (C) 2013 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// <boost/thread/synchronized_value.hpp>

// class synchronized_value<T,M>

// strict_lock_ptr<T,M> operator->();
// const_strict_lock_ptr<T,M> operator->() const;

#define BOOST_THREAD_VERSION 4

#include <boost/thread/synchronized_value.hpp>

#include <boost/detail/lightweight_test.hpp>

struct S {
  int f() const {return 1;}
  int g() {return 1;}
};

int main()
{
  {
      boost::synchronized_value<S> v;
      BOOST_TEST(v->f()==1);
      BOOST_TEST(v->g()==1);
  }
  {
      const boost::synchronized_value<S> v;
      BOOST_TEST(v->f()==1);
  }

  return boost::report_errors();
}

