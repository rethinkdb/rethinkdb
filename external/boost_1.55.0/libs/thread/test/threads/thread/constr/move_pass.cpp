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

// thread(thread&& t);

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

  void operator()(int i, double j)
  {
    BOOST_TEST(alive_ == 1);
    std::cout << __FILE__ << ":" << __LINE__ <<" " << n_alive << std::endl;
    //BOOST_TEST(n_alive == 1);
    BOOST_TEST(i == 5);
    BOOST_TEST(j == 5.5);
    op_run = true;
  }
};

int G::n_alive = 0;
bool G::op_run = false;

boost::thread make_thread() {
  return boost::thread(G(), 5, 5.5);
}

int main()
{
  {
    //BOOST_TEST(G::n_alive == 0);
    BOOST_TEST(!G::op_run);
    boost::thread t0((G()));
    boost::thread::id id = t0.get_id();
    boost::thread t1((boost::move(t0)));
    BOOST_TEST(t1.get_id() == id);
    BOOST_TEST(t0.get_id() == boost::thread::id());
    t1.join();
    BOOST_TEST(G::op_run);
  }
  //BOOST_TEST(G::n_alive == 0);
  {
    boost::thread t1((BOOST_THREAD_MAKE_RV_REF(make_thread())));
    t1.join();
    BOOST_TEST(G::op_run);
  }
  //BOOST_TEST(G::n_alive == 0);
  return boost::report_errors();
}
