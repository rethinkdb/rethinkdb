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


//#define BOOST_THREAD_VERSION 3
#define BOOST_THREAD_VERSION 4

#include <boost/thread/future.hpp>
#include <boost/detail/lightweight_test.hpp>

#if defined BOOST_THREAD_USES_CHRONO

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
class E : public std::exception
{
public:
  long data;
  explicit E(long i) :
    data(i)
  {
  }

  const char* what() const throw() { return ""; }

  ~E() throw() {}
};

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
    if (data_ == 0) BOOST_THROW_EXCEPTION(E(6));
    return data_;
  }
  long operator()(long i, long j) const
  {
    if (j == 'z') BOOST_THROW_EXCEPTION(E(6));
    return data_ + i + j;
  }
  ~A() {}
};

void func0(boost::packaged_task<BOOST_THREAD_DETAIL_SIGNATURE_2> p)
{
  boost::this_thread::sleep_for(boost::chrono::milliseconds(500));
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
  p(3, 'a');
#else
  p();
#endif
}

void func1(boost::packaged_task<BOOST_THREAD_DETAIL_SIGNATURE_2> p)
{
  boost::this_thread::sleep_for(boost::chrono::milliseconds(500));
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
  p(3, 'z');
#else
  p();
#endif
}

void func2(boost::packaged_task<BOOST_THREAD_DETAIL_SIGNATURE_2> p)
{
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
  p(3, 'a');
#else
  p();
#endif
  try
  {
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
  p(3, 'c');
#else
  p();
#endif
  }
  catch (const boost::future_error& e)
  {
    BOOST_TEST(e.code() == boost::system::make_error_code(boost::future_errc::promise_already_satisfied));
  }
}

void func3(boost::packaged_task<BOOST_THREAD_DETAIL_SIGNATURE_2> p)
{
  try
  {
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
  p(3, 'a');
#else
  p();
#endif
  }
  catch (const boost::future_error& e)
  {
    BOOST_TEST(e.code() == boost::system::make_error_code(boost::future_errc::no_state));
  }
}

int main()
{
  {
    boost::packaged_task<BOOST_THREAD_DETAIL_SIGNATURE_2> p(A(5));
    boost::future<double> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
    boost::thread(func0, boost::move(p)).detach();
#else
    //p();
#endif
    //BOOST_TEST(f.get() == 5.0);
  }
  {
    boost::packaged_task<BOOST_THREAD_DETAIL_SIGNATURE_2> p(A(0));
    boost::future<double> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
    boost::thread(func1, boost::move(p)).detach();
#endif
    try
    {
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
#else
      p();
#endif
      f.get();
      BOOST_TEST(false);
    }
    catch (const E& e)
    {
      BOOST_TEST(e.data == 6);
    }
    catch (...)
    {
      BOOST_TEST(false);
    }
  }
  {
    boost::packaged_task<BOOST_THREAD_DETAIL_SIGNATURE_2> p(A(5));
    boost::future<double> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
    boost::thread t(func2, boost::move(p));
#else
    p();
#endif
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
    BOOST_TEST(f.get() == 105);
    t.join();
#else
    BOOST_TEST(f.get() == 5.0);
#endif
  }
  {
    boost::packaged_task<BOOST_THREAD_DETAIL_SIGNATURE_2> p;
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
    boost::thread t(func3, boost::move(p));
    t.join();
#else
    try
    {
  #if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
    p(3, 'a');
  #else
    p();
  #endif
    }
    catch (const boost::future_error& e)
    {
      BOOST_TEST(e.code() == boost::system::make_error_code(boost::future_errc::no_state));
    }
#endif
  }

  return boost::report_errors();
}

#else
#error "Test not applicable: BOOST_THREAD_USES_CHRONO not defined for this platform as not supported"
#endif
