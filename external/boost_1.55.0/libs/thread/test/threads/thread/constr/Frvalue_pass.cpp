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

#define BOOST_THREAD_USES_MOVE

#include <boost/thread/thread_only.hpp>
#include <new>
#include <cstdlib>
#include <cassert>
#include <boost/detail/lightweight_test.hpp>
#include <boost/static_assert.hpp>

class MoveOnly
{
public:
  BOOST_THREAD_MOVABLE_ONLY(MoveOnly)
  MoveOnly()
  {
  }
  MoveOnly(BOOST_THREAD_RV_REF(MoveOnly))
  {}

  void operator()()
  {
  }
};

MoveOnly MakeMoveOnly() {
  return BOOST_THREAD_MAKE_RV_REF(MoveOnly());
}

#if defined  BOOST_NO_CXX11_RVALUE_REFERENCES && defined BOOST_THREAD_USES_MOVE
BOOST_STATIC_ASSERT(::boost::is_function<boost::rv<boost::rv<MoveOnly> >&>::value==false);
#endif

int main()
{
  {
    boost::thread t(( BOOST_THREAD_MAKE_RV_REF(MakeMoveOnly()) ));
    t.join();
  }
  return boost::report_errors();
}
