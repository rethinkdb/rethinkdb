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

// class future<R>

// const R& future::get();
// R& future<R&>::get();
// void future<void>::get();

//#define BOOST_THREAD_VERSION 3
#define BOOST_THREAD_VERSION 4
#define BOOST_THREAD_USES_LOG
#define BOOST_THREAD_USES_LOG_THREAD_ID
#include <boost/thread/detail/log.hpp>

#include <boost/thread/future.hpp>
#include <boost/thread/thread.hpp>
#include <boost/detail/lightweight_test.hpp>

#if defined BOOST_THREAD_USES_CHRONO

namespace boost
{
template <typename T>
struct wrap
{
  wrap(T const& v) : value(v){}
  T value;

};

template <typename T>
exception_ptr make_exception_ptr(T v) {
  return copy_exception(wrap<T>(v));
}
}

void func1(boost::promise<int> p)
{
    boost::this_thread::sleep_for(boost::chrono::milliseconds(500));
    p.set_value(3);
}

void func2(boost::promise<int> p)
{
    boost::this_thread::sleep_for(boost::chrono::milliseconds(500));
    p.set_exception(boost::make_exception_ptr(3));
}

int j = 0;

void func3(boost::promise<int&> p)
{
    boost::this_thread::sleep_for(boost::chrono::milliseconds(500));
    j = 5;
    p.set_value(j);
}

void func4(boost::promise<int&> p)
{
    boost::this_thread::sleep_for(boost::chrono::milliseconds(500));
    p.set_exception(boost::make_exception_ptr(3.5));
}

void func5(boost::promise<void> p)
{
    boost::this_thread::sleep_for(boost::chrono::milliseconds(500));
    p.set_value();
}

void func6(boost::promise<void> p)
{
    boost::this_thread::sleep_for(boost::chrono::milliseconds(500));
    p.set_exception(boost::make_exception_ptr(4));
}


int main()
{
  BOOST_THREAD_LOG << BOOST_THREAD_END_LOG;
  {
      typedef int T;
      {
          boost::promise<T> p;
          boost::future<T> f = p.get_future();
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
           boost::thread(func1, boost::move(p)).detach();
#else
           p.set_value(3);
#endif
          BOOST_TEST(f.valid());
          BOOST_TEST(f.get() == 3);
#ifdef BOOST_THREAD_PROVIDES_FUTURE_INVALID_AFTER_GET
          BOOST_TEST(!f.valid());
#endif
      }
      BOOST_THREAD_LOG << BOOST_THREAD_END_LOG;
      {
          boost::promise<T> p;
          BOOST_THREAD_LOG << BOOST_THREAD_END_LOG;
          boost::future<T> f = p.get_future();
          BOOST_THREAD_LOG << BOOST_THREAD_END_LOG;
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
          boost::thread(func2, boost::move(p)).detach();
#else
          p.set_exception(boost::make_exception_ptr(3));
#endif
          BOOST_THREAD_LOG << BOOST_THREAD_END_LOG;
          try
          {
            BOOST_THREAD_LOG << BOOST_THREAD_END_LOG;
              BOOST_TEST(f.valid());
              BOOST_THREAD_LOG << BOOST_THREAD_END_LOG;
              BOOST_TEST(f.get() == 3);
              BOOST_THREAD_LOG << BOOST_THREAD_END_LOG;
              BOOST_TEST(false);
              BOOST_THREAD_LOG << BOOST_THREAD_END_LOG;
          }
          catch (boost::wrap<int> const& i)
          {
            BOOST_THREAD_LOG << BOOST_THREAD_END_LOG;
              BOOST_TEST(i.value == 3);
              BOOST_THREAD_LOG << BOOST_THREAD_END_LOG;
          }
          catch (...)
          {
            BOOST_THREAD_LOG << BOOST_THREAD_END_LOG;
              BOOST_TEST(false);
              BOOST_THREAD_LOG << BOOST_THREAD_END_LOG;
          }
#ifdef BOOST_THREAD_PROVIDES_FUTURE_INVALID_AFTER_GET
          BOOST_THREAD_LOG << BOOST_THREAD_END_LOG;
          BOOST_TEST(!f.valid());
#endif
          BOOST_THREAD_LOG << BOOST_THREAD_END_LOG;
      }
  }
  BOOST_THREAD_LOG << BOOST_THREAD_END_LOG;
  {
      typedef int& T;
      {
          boost::promise<T> p;
          boost::future<T> f = p.get_future();
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
          boost::thread(func3, boost::move(p)).detach();
#else
          int j=5;
          p.set_value(j);
#endif
          BOOST_TEST(f.valid());
          BOOST_TEST(f.get() == 5);
#ifdef BOOST_THREAD_PROVIDES_FUTURE_INVALID_AFTER_GET
          BOOST_TEST(!f.valid());
#endif
      }
      BOOST_THREAD_LOG << BOOST_THREAD_END_LOG;
      {
          boost::promise<T> p;
          boost::future<T> f = p.get_future();
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
          boost::thread(func4, boost::move(p)).detach();
#else
          p.set_exception(boost::make_exception_ptr(3.5));
#endif
          try
          {
              BOOST_TEST(f.valid());
              BOOST_TEST(f.get() == 3);
              BOOST_TEST(false);
          }
          catch (boost::wrap<double> const& i)
          {
              BOOST_TEST(i.value == 3.5);
          }
#ifdef BOOST_THREAD_PROVIDES_FUTURE_INVALID_AFTER_GET
          BOOST_TEST(!f.valid());
#endif
      }
      BOOST_THREAD_LOG << BOOST_THREAD_END_LOG;
      {
          boost::promise<T> p;
          boost::future<T> f = p.get_future();
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
          boost::thread(func4, boost::move(p)).detach();
#else
          p.set_exception(boost::make_exception_ptr(3.5));
#endif
          try
          {
              BOOST_TEST(f.valid());
              boost::exception_ptr ptr = f.get_exception_ptr();
          }
          catch (...)
          {
            BOOST_TEST(false);
          }
          BOOST_TEST(f.valid());
      }
  }
  BOOST_THREAD_LOG << BOOST_THREAD_END_LOG;

  typedef void T;
  {
      boost::promise<T> p;
      boost::future<T> f = p.get_future();
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
       boost::thread(func5, boost::move(p)).detach();
#else
       p.set_value();
#endif
      BOOST_TEST(f.valid());
      f.get();
#ifdef BOOST_THREAD_PROVIDES_FUTURE_INVALID_AFTER_GET
      BOOST_TEST(!f.valid());
#endif
  }
  BOOST_THREAD_LOG << BOOST_THREAD_END_LOG;
  {
      boost::promise<T> p;
      boost::future<T> f = p.get_future();
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
      boost::thread(func6, boost::move(p)).detach();
#else
      p.set_exception(boost::make_exception_ptr(4));
#endif
      try
      {
          BOOST_TEST(f.valid());
          f.get();
          BOOST_TEST(false);
      }
      catch (boost::wrap<int> const& i)
      {
          BOOST_TEST(i.value == 4);
      }
      catch (...)
      {
          BOOST_TEST(false);
      }
#ifdef BOOST_THREAD_PROVIDES_FUTURE_INVALID_AFTER_GET
      BOOST_TEST(!f.valid());
#endif
  }
  BOOST_THREAD_LOG << BOOST_THREAD_END_LOG;

  return boost::report_errors();
}

#else
#error "Test not applicable: BOOST_THREAD_USES_CHRONO not defined for this platform as not supported"
#endif
