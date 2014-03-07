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

// thread& operator=(thread&& t);

#define BOOST_THREAD_PROVIDES_THREAD_MOVE_ASSIGN_CALLS_TERMINATE_IF_JOINABLE

#include <boost/thread/thread_only.hpp>
#include <new>
#include <cstdlib>
#include <cassert>
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

  void operator()(int i, double j)
  {
    BOOST_TEST(alive_ == 1);
    //BOOST_TEST(n_alive == 1);
    BOOST_TEST(i == 5);
    BOOST_TEST(j == 5.5);
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
    boost::thread t0(G(), 5, 5.5);
    boost::thread::id id = t0.get_id();
    boost::thread t1;
    t1 = boost::move(t0);
    BOOST_TEST(t1.get_id() == id);
    BOOST_TEST(t0.get_id() == boost::thread::id());
    t1.join();
    BOOST_TEST(G::op_run);
  }
  BOOST_TEST(G::n_alive == 0);
  {
    boost::thread t0(G(), 5, 5.5);
    boost::thread t1;
    t0 = boost::move(t1);
    BOOST_TEST(false);
  }
  return boost::report_errors();
}

