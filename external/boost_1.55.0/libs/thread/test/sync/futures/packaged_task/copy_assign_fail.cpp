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

// packaged_task& operator=(packaged_task&) = delete;


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
      p = p0;
  }

  return boost::report_errors();
}

#include "../../../remove_error_code_unused_warning.hpp"

