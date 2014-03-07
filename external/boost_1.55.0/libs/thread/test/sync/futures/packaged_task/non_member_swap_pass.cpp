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

// template <class R>
//   void
//   swap(packaged_task<R>& x, packaged_task<R>& y);

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
    explicit A(long i) : data_(i) {}

    long operator()() const {return data_;}
    long operator()(long i, long j) const {return data_ + i + j;}
};

int main()
{
  {
      boost::packaged_task<BOOST_THREAD_DETAIL_SIGNATURE> p0(A(5));
      boost::packaged_task<BOOST_THREAD_DETAIL_SIGNATURE> p;
      p.swap(p0);
      BOOST_TEST(!p0.valid());
      BOOST_TEST(p.valid());
      boost::future<double> f = BOOST_THREAD_MAKE_RV_REF(p.get_future());
      //p(3, 'a');
      p();
      BOOST_TEST(f.get() == 5.0);
  }
  {
      boost::packaged_task<BOOST_THREAD_DETAIL_SIGNATURE> p0;
      boost::packaged_task<BOOST_THREAD_DETAIL_SIGNATURE> p;
      p.swap(p0);
      BOOST_TEST(!p0.valid());
      BOOST_TEST(!p.valid());
  }

}

