//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// Copyright (C) 2011 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// <boost/thread/future.hpp>

// class promise<R>

// void promise<R&>::set_value_at_thread_exit(R& r);

#define BOOST_THREAD_VERSION 4

#include <boost/thread/future.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <memory>

int i = 0;

//void func(boost::promise<int&> p)
boost::promise<int&> p;
void func()
{
  p.set_value_at_thread_exit(i);
  i = 4;
}

int main()
{
  {
    //boost::promise<int&> p;
    boost::future<int&> f = p.get_future();
    //boost::thread(func, boost::move(p)).detach();
    boost::thread(func).detach();
    int r = f.get();
    BOOST_TEST(r == 4);
  }
  return boost::report_errors();
}

