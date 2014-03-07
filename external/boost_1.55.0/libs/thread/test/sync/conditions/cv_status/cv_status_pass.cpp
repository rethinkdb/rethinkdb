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

// <boost/thread/thread.hpp>

// class thread

// static unsigned hardware_concurrency();

#include <boost/thread/condition_variable.hpp>
#include <boost/detail/lightweight_test.hpp>

int main()
{
  {
    BOOST_TEST(boost::cv_status::no_timeout != boost::cv_status::timeout);
  }
  {
    boost::cv_status st = boost::cv_status::no_timeout;
    BOOST_TEST(st == boost::cv_status::no_timeout);
    BOOST_TEST(boost::cv_status::no_timeout==st);
    BOOST_TEST(st != boost::cv_status::timeout);
    BOOST_TEST(boost::cv_status::timeout!=st);
  }
  {
    boost::cv_status st = boost::cv_status::timeout;
    BOOST_TEST(st == boost::cv_status::timeout);
    BOOST_TEST(boost::cv_status::timeout==st);
    BOOST_TEST(st != boost::cv_status::no_timeout);
    BOOST_TEST(boost::cv_status::no_timeout!=st);
  }
  {
    boost::cv_status st;
    st = boost::cv_status::no_timeout;
    BOOST_TEST(st == boost::cv_status::no_timeout);
    BOOST_TEST(boost::cv_status::no_timeout==st);
    BOOST_TEST(st != boost::cv_status::timeout);
    BOOST_TEST(boost::cv_status::timeout!=st);
  }
  {
    boost::cv_status st;
    st = boost::cv_status::timeout;
    BOOST_TEST(st == boost::cv_status::timeout);
    BOOST_TEST(boost::cv_status::timeout==st);
    BOOST_TEST(st != boost::cv_status::no_timeout);
    BOOST_TEST(boost::cv_status::no_timeout!=st);
  }
  return boost::report_errors();
}

