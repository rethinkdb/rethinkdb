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

// ~packaged_task();

//#define BOOST_THREAD_VERSION 3
#define BOOST_THREAD_VERSION 4


#include <boost/thread/future.hpp>
#include <boost/thread/thread.hpp>
#include <boost/detail/lightweight_test.hpp>

#if BOOST_THREAD_VERSION == 4
#define BOOST_THREAD_DETAIL_SIGNATURE double()
#else
#define BOOST_THREAD_DETAIL_SIGNATURE double
#endif

#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK
#if defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
#define BOOST_THREAD_DETAIL_SIGNATURE_2 double(int, char)
#define BOOST_THREAD_DETAIL_SIGNATURE_2_RES 5 + 3 +'a'
#else
#define BOOST_THREAD_DETAIL_SIGNATURE_2 double()
#define BOOST_THREAD_DETAIL_SIGNATURE_2_RES 5
#endif
#else
#define BOOST_THREAD_DETAIL_SIGNATURE_2 double
#define BOOST_THREAD_DETAIL_SIGNATURE_2_RES 5
#endif

class A
{
    long data_;

public:
    explicit A(long i) : data_(i) {}

    long operator()() const {return data_;}
    long operator()(long i, long j) const {return data_ + i + j;}
};

void func(boost::packaged_task<BOOST_THREAD_DETAIL_SIGNATURE> )
{
}

void func2(boost::packaged_task<BOOST_THREAD_DETAIL_SIGNATURE_2> p)
{
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
  p(3, 'a');
#else
  p();
#endif
}

int main()
{
  {
      boost::packaged_task<BOOST_THREAD_DETAIL_SIGNATURE> p(A(5));
      boost::future<double> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
      boost::thread(func, boost::move(p)).detach();
#else
      boost::packaged_task<BOOST_THREAD_DETAIL_SIGNATURE>* p2=new boost::packaged_task<BOOST_THREAD_DETAIL_SIGNATURE>(boost::move(p));
      delete p2;
#endif
      try
      {
          f.get();
          BOOST_TEST(false);
      }
      catch (const boost::future_error& e)
      {
          BOOST_TEST(e.code() == boost::system::make_error_code(boost::future_errc::broken_promise));
      }
  }
  {
      std::cout << __LINE__ << std::endl;
      boost::packaged_task<BOOST_THREAD_DETAIL_SIGNATURE_2> p(A(5));
      boost::future<double> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
      boost::thread(func2, boost::move(p)).detach();
#else
      p();
#endif
      std::cout << __LINE__ << std::endl;
      BOOST_TEST(f.get() == BOOST_THREAD_DETAIL_SIGNATURE_2_RES);
      std::cout << __LINE__ << std::endl;
  }
  return boost::report_errors();
}

