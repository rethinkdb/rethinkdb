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

// ~thread();

#define BOOST_THREAD_PROVIDES_THREAD_DESTRUCTOR_CALLS_TERMINATE_IF_JOINABLE

#include <boost/thread/thread_only.hpp>
#include <new>
#include <cstdlib>
#include <boost/detail/lightweight_test.hpp>

class G
{
  int alive_;
public:
  static int n_alive;
  static bool op_run;

  G() :
    alive_(1)
  {
    ++n_alive;
  }
  G(const G& g) :
    alive_(g.alive_)
  {
    ++n_alive;
  }
  ~G()
  {
    alive_ = 0;
    --n_alive;
  }

  void operator()()
  {
    BOOST_TEST(alive_ == 1);
    //BOOST_TEST(n_alive == 1);
    op_run = true;
  }
};

int G::n_alive = 0;
bool G::op_run = false;

void f1()
{
  std::exit(boost::report_errors());
}

int main()
{
  std::set_terminate(f1);
  {
    BOOST_TEST(G::n_alive == 0);
    BOOST_TEST(!G::op_run);
    boost::thread t( (G()));
#if defined BOOST_THREAD_USES_CHRONO
    boost::this_thread::sleep_for(boost::chrono::milliseconds(250));
#endif
    BOOST_TEST(t.joinable());
  }
  BOOST_TEST(false);
  return boost::report_errors();
}

