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

// future<R> get_future();

#define BOOST_THREAD_VERSION 3

#include <boost/thread/future.hpp>
#include <boost/detail/lightweight_test.hpp>

int main()
{
  {
      boost::promise<double> p;
      boost::future<double> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
      p.set_value(105.5);
      BOOST_TEST(f.get() == 105.5);
  }
  {
      boost::promise<double> p;
      boost::future<double> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
      try
      {
          f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
          BOOST_TEST(false);
      }
      catch (const boost::future_error& e)
      {
          BOOST_TEST(e.code() ==  boost::system::make_error_code(boost::future_errc::future_already_retrieved));
      }
  }
  {
      boost::promise<double> p;
      boost::promise<double> p0 = boost::move(p);
      try
      {
          boost::future<double> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
          BOOST_TEST(false);
      }
      catch (const boost::future_error& e)
      {
          BOOST_TEST(e.code() ==  boost::system::make_error_code(boost::future_errc::no_state));
      }
  }


  return boost::report_errors();
}

