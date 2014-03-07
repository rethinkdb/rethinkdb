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

// packaged_task(packaged_task&& other);

#define BOOST_THREAD_VERSION 4

#include <boost/thread/future.hpp>
#include <boost/detail/lightweight_test.hpp>

#if defined BOOST_THREAD_USES_CHRONO && \
    defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && \
    defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)

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

  long operator()(long i, long j) const
  {
    if (j == 'z') BOOST_THROW_EXCEPTION( E(6) );
    return data_ + i + j;
  }
};

void func0_mv(BOOST_THREAD_RV_REF(boost::packaged_task<double(int, char)>) p)
//void func0(boost::packaged_task<double(int, char)> p)
{
  boost::this_thread::sleep_for(boost::chrono::milliseconds(500));
  p.make_ready_at_thread_exit(3, 'a');
}
void func0(boost::packaged_task<double(int, char)> *p)
{
  boost::this_thread::sleep_for(boost::chrono::milliseconds(500));
  p->make_ready_at_thread_exit(3, 'a');
}
void func1(boost::packaged_task<double(int, char)> *p)
{
  boost::this_thread::sleep_for(boost::chrono::milliseconds(500));
  p->make_ready_at_thread_exit(3, 'z');
}

void func2(boost::packaged_task<double(int, char)> *p)
{
  p->make_ready_at_thread_exit(3, 'a');
  try
  {
    p->make_ready_at_thread_exit(3, 'c');
  }
  catch (const boost::future_error& e)
  {
    BOOST_TEST(e.code() == boost::system::make_error_code(boost::future_errc::promise_already_satisfied));
  }
}

void func3(boost::packaged_task<double(int, char)> *p)
{
  try
  {
    p->make_ready_at_thread_exit(3, 'a');
  }
  catch (const boost::future_error& e)
  {
    BOOST_TEST(e.code() == boost::system::make_error_code(boost::future_errc::no_state));
  }
}

int main()
{
  {
    boost::packaged_task<double(int, char)> p(A(5));
    boost::future<double> f = p.get_future();
#if defined BOOST_THREAD_PROVIDES_VARIADIC_THREAD
    boost::thread(func0_mv, boost::move(p)).detach();
#else
    boost::thread(func0, &p).detach();
#endif
    BOOST_TEST(f.get() == 105.0);
  }
  {
    boost::packaged_task<double(int, char)> p2(A(5));
    boost::future<double> f = p2.get_future();
    boost::packaged_task<double(int, char)> p = boost::move(p2);
    boost::thread(func0, &p).detach();
    BOOST_TEST(f.get() == 105.0);
  }
  {
    boost::packaged_task<double(int, char)> p(A(5));
    boost::future<double> f = p.get_future();
    //boost::thread(func1, boost::move(p)).detach();
    boost::thread(func1, &p).detach();
    try
    {
      f.get();
      BOOST_TEST(false);
    }
    catch (const E& e)
    {
      BOOST_TEST(e.data == 6);
    }
  }
  {
    boost::packaged_task<double(int, char)> p2(A(5));
    boost::future<double> f = p2.get_future();
    boost::packaged_task<double(int, char)> p = boost::move(p2);
    boost::thread(func1, &p).detach();
    try
    {
      f.get();
      BOOST_TEST(false);
    }
    catch (const E& e)
    {
      BOOST_TEST(e.data == 6);
    }
  }
  {
    boost::packaged_task<double(int, char)> p(A(5));
    boost::future<double> f = p.get_future();
    //boost::thread(func2, boost::move(p)).detach();
    boost::thread(func2, &p).detach();
    BOOST_TEST(f.get() == 105.0);
  }
  {
    boost::packaged_task<double(int, char)> p2(A(5));
    boost::future<double> f = p2.get_future();
    boost::packaged_task<double(int, char)> p = boost::move(p2);
    boost::thread(func2, &p).detach();
    BOOST_TEST(f.get() == 105.0);
  }
  {
    boost::packaged_task<double(int, char)> p(A(5));
    //boost::thread t(func3, boost::move(p));
    boost::thread t(func3, &p);
    t.join();
  }
  {
    boost::packaged_task<double(int, char)> p2(A(5));
    boost::packaged_task<double(int, char)> p = boost::move(p2);
    boost::thread t(func3, &p);
    t.join();
  }

  return boost::report_errors();
}

#else
int main()
{
  return boost::report_errors();
}
//#error "Test not applicable: BOOST_THREAD_USES_CHRONO not defined for this platform as not supported"
#endif


