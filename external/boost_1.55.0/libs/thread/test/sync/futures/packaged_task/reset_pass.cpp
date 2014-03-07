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
// class packaged_task<R>

// void operator()();


#define BOOST_THREAD_VERSION 4
#if BOOST_THREAD_VERSION == 4
#define BOOST_THREAD_DETAIL_SIGNATURE double()
#else
#define BOOST_THREAD_DETAIL_SIGNATURE double
#endif

#include <boost/thread/future.hpp>
#include <boost/detail/lightweight_test.hpp>

class A
{
  long data_;

public:
  explicit A(long i) :
    data_(i)
  {
  }

  long operator()() const
  {
    return data_;
  }
  long operator()(long i, long j) const
  {
    if (j == 'z') throw A(6);
    return data_ + i + j;
  }
};

int main()
{
  {
    boost::packaged_task<BOOST_THREAD_DETAIL_SIGNATURE> p(A(5));
    boost::future<double> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
    //p(3, 'a');
    p();
    BOOST_TEST(f.get() == 5.0);
    p.reset();
    //p(4, 'a');
    p();
    f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
    BOOST_TEST(f.get() == 5.0);
  }
  {
    boost::packaged_task<BOOST_THREAD_DETAIL_SIGNATURE> p;
    try
    {
      p.reset();
      BOOST_TEST(false);
    }
    catch (const boost::future_error& e)
    {
      BOOST_TEST(e.code() == boost::system::make_error_code(boost::future_errc::no_state));
    }
  }

  return boost::report_errors();
}

