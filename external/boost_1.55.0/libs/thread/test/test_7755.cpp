// Copyright (C) 2013 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

////////////////////////////////////////////

//#define BOOST_THREAD_PROVIDES_GENERIC_SHARED_MUTEX_ON_WIN
#include <iostream>
#include <boost/thread/thread_only.hpp>
#include <boost/thread/shared_mutex.hpp>
// shared_mutex_deadlock.cpp : Defines the entry point for the console application.
//


boost::shared_mutex mutex;

void thread2_func()
{
  int i (0);
  while (true)
  {
    if (mutex.timed_lock(boost::posix_time::milliseconds(500)))
    {
      std::cout << "Unique lock acquired" << std::endl
        << "Test successful" << std::endl;
      mutex.unlock();
      break;
    }
    ++i;
    if (i == 100)
    {
      std::cout << "Test failed. App is deadlocked" << std::endl;
      break;
    }
    boost::this_thread::sleep(boost::posix_time::seconds(1));
  }
}

void thread3_func()
{
  boost::shared_lock<boost::shared_mutex> lock (mutex);
  std::cout << "Shared lock acquired" << std::endl
        << "Test successful" << std::endl;
}

void thread3_func_workaround()
{
  while (true)
  {
    if (mutex.timed_lock_shared(boost::posix_time::milliseconds(200)))
    {
      std::cout << "Shared lock acquired" << std::endl
        << "Test successful" << std::endl;
      mutex.unlock_shared();
      break;
    }
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
  }
}

int main()
{
  std::cout << "Starting" << std::endl;

  // 1 - lock the mutex
  boost::shared_lock<boost::shared_mutex> lock (mutex);

  // 2 - start thread#2
  boost::thread thread2(&thread2_func);

  // 3 - sleep
  boost::this_thread::sleep(boost::posix_time::milliseconds(10));

  std::cout << "Thread#2 is waiting" << std::endl;

  // - start thread3
  boost::thread thread3(&thread3_func);
  //boost::thread thread3(&thread3_func_workaround);

  std::cout << "Thread#3 is started and blocked. It never will waked" << std::endl;

  thread3.join(); // will never return

  lock.unlock(); // release shared ownership. thread#2 will take unique ownership

  thread2.join();

  std::cout << "Finished" << std::endl;
  return 0;
}



