// (C) Copyright 2009-2012 Anthony Williams
// (C) Copyright 2012 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_THREAD_VERSION 3

#include <iostream>
#include <boost/thread/scoped_thread.hpp>

void do_something(int& i)
{
  ++i;
}
void f(int, int)
{
}

struct func
{
  int& i;

  func(int& i_) :
    i(i_)
  {
  }

  void operator()()
  {
    for (unsigned j = 0; j < 1000000; ++j)
    {
      do_something(i);
    }
  }
};

void do_something_in_current_thread()
{
}

//void do_something_with_current_thread(boost::thread&& th)
//{
//  th.join();
//}

int main()
{
  {
    int some_local_state;
    boost::strict_scoped_thread<> t( (boost::thread(func(some_local_state))));

    do_something_in_current_thread();
  }
  {
    int some_local_state;
    boost::thread t(( func(some_local_state) ));
    boost::strict_scoped_thread<> g( (boost::move(t)) );

    do_something_in_current_thread();
  }
//  {
//    int some_local_state;
//    boost::thread t(( func(some_local_state) ));
//    boost::strict_scoped_thread<> g( (boost::move(t)) );
//
//    do_something_in_current_thread();
//    do_something_with_current_thread(boost::thread(g));
//  }
  {
    int some_local_state;
    boost::scoped_thread<> t( (boost::thread(func(some_local_state))));

    if (t.joinable())
      t.join();
    else
      do_something_in_current_thread();
  }
  {
    int some_local_state;
    boost::thread t(( func(some_local_state) ));
    boost::scoped_thread<> g( (boost::move(t)) );
    t.detach();

    do_something_in_current_thread();
  }
  {
    boost::scoped_thread<> g( &f, 1, 2 );
    do_something_in_current_thread();
  }
  return 0;
}

