// Copyright (C) 2013 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// <boost/thread/synchronized_value.hpp>

// class synchronized_value<T,M>

// synchronized_value(T const&);

#define BOOST_THREAD_VERSION 4

#include <boost/thread/synchronized_value.hpp>

#include <boost/detail/lightweight_test.hpp>

int main()
{

  {
    int i = 1;
    boost::synchronized_value<int> v(i);
    BOOST_TEST(v.value() == 1);
  }

  return boost::report_errors();
}

