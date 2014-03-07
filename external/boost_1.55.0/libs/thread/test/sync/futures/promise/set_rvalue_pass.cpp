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

// void promise::set_value(R&& r);

#define BOOST_THREAD_VERSION 3

#include <boost/thread/future.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/static_assert.hpp>

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

struct A
{
  A() :
    value(0)
  {
  }
  A(int i) :
    value(i)
  {
  }
  BOOST_THREAD_DELETE_COPY_CTOR(A)
  A(A&& rhs)
  {
    if(rhs.value==0)
    throw 9;
    else
    {
      value=rhs.value;
      rhs.value=0;
    }
  }
  int value;
};

#endif  // BOOST_NO_CXX11_RVALUE_REFERENCES
int main()
{

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

  {
    typedef A T;
    T i;
    boost::promise<T> p;
    boost::future<T> f = p.get_future();
    try
    {
      p.set_value(boost::move(i));
      BOOST_TEST(false);
    }
    catch (int j)
    {
      BOOST_TEST(j == 9);
    }
    catch (...)
    {
      BOOST_TEST(false);
    }
  }
  {
    typedef A T;
    T i(3);
    boost::promise<T> p;
    boost::future<T> f = p.get_future();
    p.set_value(boost::move(i));
    BOOST_TEST(f.get().value == 3);
    try
    {
      T j(3);
      p.set_value(boost::move(j));
      BOOST_TEST(false);
    }
    catch (const boost::future_error& e)
    {
      BOOST_TEST(e.code() == boost::system::make_error_code(boost::future_errc::promise_already_satisfied));
    }
    catch (...)
    {
      BOOST_TEST(false);
    }

  }
  {
    typedef A T;
    T i(3);
    boost::promise<T> p;
    boost::future<T> f = p.get_future();
    p.set_value(boost::move(i));
    BOOST_TEST(i.value == 0);
    boost::promise<T> p2(boost::move(p));
    BOOST_TEST(f.get().value == 3);

  }
  {
    typedef A T;
    T i(3);
    boost::promise<T> p;
    boost::future<T> f = p.get_future();
    p.set_value(boost::move(i));
    BOOST_TEST(i.value == 0);
    boost::promise<T> p2(boost::move(p));
    boost::future<T> f2(boost::move(f));
    BOOST_TEST(f2.get().value == 3);

  }
  {
    typedef A T;
    T i(3);
    boost::promise<T> p;
    p.set_value(boost::move(i));
    BOOST_TEST(i.value == 0);
    boost::promise<T> p2(boost::move(p));
    boost::future<T> f = p2.get_future();
    BOOST_TEST(f.get().value == 3);

  }

  {
    typedef boost::future<int> T;
    boost::promise<int> pi;
    T fi=pi.get_future();
    pi.set_value(3);

    boost::promise<T> p;
    boost::future<T> f = p.get_future();
    p.set_value(boost::move(fi));
    boost::future<T> f2(boost::move(f));
    BOOST_TEST(f2.get().get() == 3);
  }

#endif  // BOOST_NO_CXX11_RVALUE_REFERENCES
  return boost::report_errors();
}

