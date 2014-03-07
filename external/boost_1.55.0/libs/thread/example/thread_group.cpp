// Copyright (C) 2001-2003
// William E. Kempf
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread/thread.hpp>
#include <iostream>
#include <boost/detail/lightweight_test.hpp>

int count = 0;
boost::mutex mutex;

void increment_count()
{
    boost::unique_lock<boost::mutex> lock(mutex);
    std::cout << "count = " << ++count << std::endl;
}

boost::thread_group threads2;
boost::thread* th2 = 0;

void increment_count_2()
{
    boost::unique_lock<boost::mutex> lock(mutex);
    BOOST_TEST(threads2.is_this_thread_in());
    std::cout << "count = " << ++count << std::endl;
}

int main()
{
  {
    boost::thread_group threads;
    for (int i = 0; i < 3; ++i)
        threads.create_thread(&increment_count);
    threads.join_all();
  }
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
  {
    boost::thread_group threads;
    for (int i = 0; i < 3; ++i)
        threads.create_thread(&increment_count);
    threads.interrupt_all();
    threads.join_all();
  }
#endif
  {
    boost::thread_group threads;
    boost::thread* th = new boost::thread(&increment_count);
    threads.add_thread(th);
    BOOST_TEST(! threads.is_this_thread_in());
    threads.join_all();
  }
  {
    boost::thread_group threads;
    boost::thread* th = new boost::thread(&increment_count);
    threads.add_thread(th);
    BOOST_TEST(threads.is_thread_in(th));
    threads.remove_thread(th);
    BOOST_TEST(! threads.is_thread_in(th));
    th->join();
  }
  {
    {
      boost::unique_lock<boost::mutex> lock(mutex);
      boost::thread* th2 = new boost::thread(&increment_count_2);
      threads2.add_thread(th2);
    }
    threads2.join_all();
  }
  return boost::report_errors();
}
