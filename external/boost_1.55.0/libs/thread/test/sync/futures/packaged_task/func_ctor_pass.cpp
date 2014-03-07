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

// template <class F>
//     explicit packaged_task(F&& f);


#define BOOST_THREAD_VERSION 4

#include <boost/thread/future.hpp>
#include <boost/detail/lightweight_test.hpp>

#if BOOST_THREAD_VERSION == 4
#define BOOST_THREAD_DETAIL_SIGNATURE double()
#define BOOST_THREAD_DETAIL_VOID_SIGNATURE void()
#else
#define BOOST_THREAD_DETAIL_SIGNATURE double
#define BOOST_THREAD_DETAIL_VOID_SIGNATURE void
#endif

#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK
#if defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
#define BOOST_THREAD_DETAIL_SIGNATURE_2 double(int, char)
#define BOOST_THREAD_DETAIL_SIGNATURE_2_RES 5 + 3 +'a'
#define BOOST_THREAD_DETAIL_VOID_SIGNATURE_2 void(int)
#else
#define BOOST_THREAD_DETAIL_SIGNATURE_2 double()
#define BOOST_THREAD_DETAIL_SIGNATURE_2_RES 5
#define BOOST_THREAD_DETAIL_VOID_SIGNATURE_2 void()
#endif
#else
#define BOOST_THREAD_DETAIL_SIGNATURE_2 double
#define BOOST_THREAD_DETAIL_SIGNATURE_2_RES 5
#define BOOST_THREAD_DETAIL_VOID_SIGNATURE_2 void
#endif

void void_fct()
{
  return;
}
double fct()
{
  return 5.0;
}
long lfct()
{
  return 5;
}

class A
{
public:
  long data_;

  static int n_moves;
  static int n_copies;
  BOOST_THREAD_COPYABLE_AND_MOVABLE(A)
  static void reset()
  {
    n_moves=0;
    n_copies=0;
  }

  explicit A(long i) : data_(i)
  {
  }
  A(BOOST_THREAD_RV_REF(A) a) : data_(BOOST_THREAD_RV(a).data_)
  {
    BOOST_THREAD_RV(a).data_ = -1;
    ++n_moves;
  }
  A& operator=(BOOST_THREAD_RV_REF(A) a)
  {
    data_ = BOOST_THREAD_RV(a).data_;
    BOOST_THREAD_RV(a).data_ = -1;
    ++n_moves;
    return *this;
  }
  A(const A& a) : data_(a.data_)
  {
    ++n_copies;
  }
  A& operator=(A const& a)
  {
    data_ = a.data_;
    ++n_copies;
    return *this;
  }
  ~A()
  {
  }

  void operator()(int) const
  { }
  long operator()() const
  { return data_;}
  long operator()(long i, long j) const
  { return data_ + i + j;}
};

int A::n_moves = 0;
int A::n_copies = 0;

class M
{

public:
  long data_;
  static int n_moves;

  BOOST_THREAD_MOVABLE_ONLY(M)
  static void reset() {
    n_moves=0;
  }
  explicit M(long i) : data_(i)
  {
  }
  M(BOOST_THREAD_RV_REF(M) a) : data_(BOOST_THREAD_RV(a).data_)
  {
    BOOST_THREAD_RV(a).data_ = -1;
    ++n_moves;
  }
  M& operator=(BOOST_THREAD_RV_REF(M) a)
  {
    data_ = BOOST_THREAD_RV(a).data_;
    BOOST_THREAD_RV(a).data_ = -1;
    ++n_moves;
    return *this;
  }
  ~M()
  {
  }

  void operator()(int) const
  { }
  long operator()() const
  { return data_;}
  long operator()(long i, long j) const
  { return data_ + i + j;}
};

int M::n_moves = 0;

class C
{
public:
  long data_;

  static int n_copies;
  static void reset()
  {
    n_copies=0;
  }

  explicit C(long i) : data_(i)
  {
  }
  C(const C& a) : data_(a.data_)
  {
    ++n_copies;
  }
  C& operator=(C const& a)
  {
    data_ = a.data_;
    ++n_copies;
    return *this;
  }
  ~C()
  {
  }

  void operator()(int) const
  { }
  long operator()() const
  { return data_;}
  long operator()(long i, long j) const
  { return data_ + i + j;}
};
int C::n_copies = 0;

int main()
{
  {
      A::reset();
      boost::packaged_task<BOOST_THREAD_DETAIL_SIGNATURE_2> p(BOOST_THREAD_MAKE_RV_REF(A(5)));
      BOOST_TEST(p.valid());
      boost::future<double> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
      p(3, 'a');
#else
      p();
#endif
      BOOST_TEST(f.get() == BOOST_THREAD_DETAIL_SIGNATURE_2_RES);
      BOOST_TEST(A::n_copies == 0);
      BOOST_TEST_EQ(A::n_moves, 1);
  }
  {
    A::reset();
      A a(5);
      boost::packaged_task<BOOST_THREAD_DETAIL_SIGNATURE> p(a);
      BOOST_TEST(p.valid());
      boost::future<double> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
      //p(3, 'a');
      p();
      BOOST_TEST(f.get() == 5.0);
      BOOST_TEST_EQ(A::n_copies, 1);
      BOOST_TEST_EQ(A::n_moves, 0);
  }
  {
    A::reset();
      const A a(5);
      boost::packaged_task<BOOST_THREAD_DETAIL_SIGNATURE> p(a);
      BOOST_TEST(p.valid());
      boost::future<double> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
      //p(3, 'a');
      p();
      BOOST_TEST(f.get() == 5.0);
      BOOST_TEST_EQ(A::n_copies, 1);
      BOOST_TEST_EQ(A::n_moves, 0);
  }
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
  {
    A::reset();
      boost::packaged_task<BOOST_THREAD_DETAIL_VOID_SIGNATURE_2> p(BOOST_THREAD_MAKE_RV_REF(A(5)));
      BOOST_TEST(p.valid());
      boost::future<void> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
      p(1);
      BOOST_TEST(A::n_copies == 0);
      BOOST_TEST_EQ(A::n_moves, 1);
  }
  {
    A::reset();
      A a(5);
      boost::packaged_task<BOOST_THREAD_DETAIL_VOID_SIGNATURE_2> p(a);
      BOOST_TEST(p.valid());
      boost::future<void> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
      p(1);
      BOOST_TEST_EQ(A::n_copies, 1);
      BOOST_TEST_EQ(A::n_moves, 0);
  }
  {
    A::reset();
      const A a(5);
      boost::packaged_task<BOOST_THREAD_DETAIL_VOID_SIGNATURE_2> p(a);
      BOOST_TEST(p.valid());
      boost::future<void> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
      p(1);
      BOOST_TEST_EQ(A::n_copies, 1);
      BOOST_TEST_EQ(A::n_moves, 0);
  }
#endif
  {
    M::reset();
      boost::packaged_task<BOOST_THREAD_DETAIL_SIGNATURE_2> p(BOOST_THREAD_MAKE_RV_REF(M(5)));
      BOOST_TEST(p.valid());
      boost::future<double> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
      p(3, 'a');
#else
      p();
#endif
      BOOST_TEST(f.get() == BOOST_THREAD_DETAIL_SIGNATURE_2_RES);
      BOOST_TEST_EQ(M::n_moves, 1);
  }
  {
    M::reset();
      M a(5);
      boost::packaged_task<BOOST_THREAD_DETAIL_SIGNATURE> p(boost::move(a));
      BOOST_TEST(p.valid());
      boost::future<double> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
      //p(3, 'a');
      p();
      BOOST_TEST(f.get() == 5.0);
      BOOST_TEST_EQ(M::n_moves, 1);
  }
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
  {
    M::reset();
      boost::packaged_task<BOOST_THREAD_DETAIL_VOID_SIGNATURE_2> p(BOOST_THREAD_MAKE_RV_REF(M(5)));
      BOOST_TEST(p.valid());
      boost::future<void> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
      p(1);
      BOOST_TEST_EQ(M::n_moves, 1);
  }
  {
    M::reset();
      M a(5);
      boost::packaged_task<BOOST_THREAD_DETAIL_VOID_SIGNATURE_2> p(boost::move(a));
      BOOST_TEST(p.valid());
      boost::future<void> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
      p(1);
      BOOST_TEST_EQ(M::n_moves, 1);
  }
#endif
  {
    C::reset();
      C a(5);
      boost::packaged_task<BOOST_THREAD_DETAIL_SIGNATURE> p(a);
      BOOST_TEST(p.valid());
      boost::future<double> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
      //p(3, 'a');
      p();
      BOOST_TEST(f.get() == 5.0);
      BOOST_TEST_EQ(C::n_copies, 1);
  }

  {
    C::reset();
      const C a(5);
      boost::packaged_task<BOOST_THREAD_DETAIL_SIGNATURE> p(a);
      BOOST_TEST(p.valid());
      boost::future<double> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
      //p(3, 'a');
      p();
      BOOST_TEST(f.get() == 5.0);
      BOOST_TEST_EQ(C::n_copies, 1);
  }
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
  {
    C::reset();
      C a(5);
      boost::packaged_task<BOOST_THREAD_DETAIL_VOID_SIGNATURE_2> p(a);
      BOOST_TEST(p.valid());
      boost::future<void> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
      p(1);
      BOOST_TEST_EQ(C::n_copies, 1);
  }

  {
    C::reset();
      const C a(5);
      boost::packaged_task<BOOST_THREAD_DETAIL_VOID_SIGNATURE_2> p(a);
      BOOST_TEST(p.valid());
      boost::future<void> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
      p(1);
      BOOST_TEST_EQ(C::n_copies, 1);
  }
#endif
  {
      boost::packaged_task<BOOST_THREAD_DETAIL_VOID_SIGNATURE> p(void_fct);
      BOOST_TEST(p.valid());
      boost::future<void> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
      p();
  }
  {
      boost::packaged_task<BOOST_THREAD_DETAIL_SIGNATURE> p(fct);
      BOOST_TEST(p.valid());
      boost::future<double> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
      //p(3, 'a');
      p();
      BOOST_TEST(f.get() == 5.0);
  }
  {
      boost::packaged_task<BOOST_THREAD_DETAIL_SIGNATURE> p(&lfct);
      BOOST_TEST(p.valid());
      boost::future<double> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
      //p(3, 'a');
      p();
      BOOST_TEST(f.get() == 5.0);
  }

  return boost::report_errors();
}

