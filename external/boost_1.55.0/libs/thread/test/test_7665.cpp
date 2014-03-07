// Copyright (C) 2010 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_THREAD_VERSION 2
#define BOOST_THREAD_USES_LOG

#include <iostream>
#include <boost/thread/thread_only.hpp>
#include <boost/thread/detail/log.hpp>

void thread()
{
  BOOST_THREAD_LOG << "<thrd" << BOOST_THREAD_END_LOG;
  try {
  boost::this_thread::sleep_for(boost::chrono::seconds(30));
  } catch (...)
  {
    BOOST_THREAD_LOG << "thrd exception" << BOOST_THREAD_END_LOG;
    throw;
  }
  //while (1)     ; // Never quit
  BOOST_THREAD_LOG << "thrd>" << BOOST_THREAD_END_LOG;
}

boost::thread example(thread);

int main()
{
  BOOST_THREAD_LOG << "<main" << BOOST_THREAD_END_LOG;
  boost::this_thread::sleep_for(boost::chrono::seconds(30));
  BOOST_THREAD_LOG << "main" << BOOST_THREAD_END_LOG;
  //while (1)     ; // Never quit
  example.join();
  BOOST_THREAD_LOG << "main>" << BOOST_THREAD_END_LOG;
  return 0;
}

