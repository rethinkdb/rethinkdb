// Copyright (C) 2013 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_THREAD_VERSION 4

#include <iostream>
#include <functional>
#include <future>

#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

int f()
{
  return 42;
}

boost::packaged_task<int()>* schedule(boost::function<int ()> const& fn)
{
  // Normally, the pointer to the packaged task is stored in a queue
  // for execution on a separate thread, and the schedule function
  // would return just a future<T>

  boost::function<int ()> copy(fn);
  boost::packaged_task<int()>* result = new boost::packaged_task<int()>(copy);
  return result;
}

struct MyFunc
{
  MyFunc(MyFunc const&) = delete;
  MyFunc& operator=(MyFunc const&) = delete;
  MyFunc() {};
  MyFunc(MyFunc &&) {};
  MyFunc& operator=(MyFunc &&) { return *this;};
  void operator()()const {}
};


int main()
{
  boost::packaged_task<int()>* p(schedule(f));
  (*p)();

  boost::future<int> fut = p->get_future();
  std::cout << "The answer to the ultimate question: " << fut.get() << std::endl;

  {
    boost::function<void()> f;
    MyFunc mf;

    boost::packaged_task<void()> t1(f);
    boost::packaged_task<void()> t2(boost::move(mf));
  }

  return 0;
}


