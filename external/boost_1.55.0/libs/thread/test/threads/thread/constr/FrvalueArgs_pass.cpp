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

// template <class F, class ...Args> thread(F&& f, Args&&... args);

#define BOOST_THREAD_VERSION 4

#include <boost/thread/thread_only.hpp>
#include <new>
#include <cstdlib>
#include <cassert>
#include <boost/detail/lightweight_test.hpp>

class MoveOnly
{
public:
  BOOST_THREAD_MOVABLE_ONLY(MoveOnly)
  MoveOnly()
  {
  }
  MoveOnly(BOOST_THREAD_RV_REF(MoveOnly))
  {}

  void operator()(BOOST_THREAD_RV_REF(MoveOnly))
  {
  }
};

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

void fct(BOOST_THREAD_RV_REF(M) v)
{
  BOOST_TEST_EQ(v.data_, 1);
}

int main()
{
#if defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
  {
    boost::thread t = boost::thread( MoveOnly(), MoveOnly() );
    t.join();
  }
  {
    M::reset();
    boost::thread t = boost::thread( fct, M(1) );
    t.join();
    BOOST_TEST_EQ(M::n_moves, 2);
  }
#endif
  return boost::report_errors();
}
