// Copyright (C) 2013 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// <boost/thread/synchronized_value.hpp>

// class synchronized_value<T,M>

// void swap(synchronized_value&,synchronized_value&);

#define BOOST_THREAD_VERSION 4

#include <boost/thread/synchronized_value.hpp>

#include <boost/detail/lightweight_test.hpp>

int main()
{

  {
    boost::synchronized_value<int> v1(1);
    boost::synchronized_value<int> v2(2);
    swap(v1,v2);
    BOOST_TEST(v1.value() == 2);
    BOOST_TEST(v2.value() == 1);
  }

  return boost::report_errors();
}

