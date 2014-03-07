// Copyright (C) 2010 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_THREAD_VERSION 2
//#define BOOST_THREAD_PROVIDES_GENERIC_SHARED_MUTEX_ON_WIN
//#define BOOST_THREAD_USES_LOG

#include <boost/thread/thread_only.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/detail/no_exceptions_support.hpp>
//#include <boost/thread/detail/log.hpp>

boost::shared_mutex mutex;

void thread()
{
  //BOOST_THREAD_LOG << "<thrd" << BOOST_THREAD_END_LOG;
  BOOST_TRY
  {
    for (int i =0; i<10; ++i)
    {
#ifndef BOOST_THREAD_USES_CHRONO
      boost::system_time timeout = boost::get_system_time() + boost::posix_time::milliseconds(50);

      if (mutex.timed_lock(timeout))
      {
        //BOOST_THREAD_LOG << "<thrd" << " i="<<i << BOOST_THREAD_END_LOG;
        boost::this_thread::sleep(boost::posix_time::milliseconds(10));
        mutex.unlock();
        //BOOST_THREAD_LOG << "<thrd" << " i="<<i << BOOST_THREAD_END_LOG;
      }
#else
      boost::chrono::system_clock::time_point timeout = boost::chrono::system_clock::now() + boost::chrono::milliseconds(50);

      //BOOST_THREAD_LOG << "<thrd" << " i="<<i << BOOST_THREAD_END_LOG;
      if (mutex.try_lock_until(timeout))
      {
        //BOOST_THREAD_LOG << "<thrd" << " i="<<i << BOOST_THREAD_END_LOG;
        boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
        mutex.unlock();
        //BOOST_THREAD_LOG << "<thrd" << " i="<<i << BOOST_THREAD_END_LOG;
      }
#endif
    }
  }
  BOOST_CATCH (boost::lock_error& le)
  {
    //BOOST_THREAD_LOG << "lock_error exception thrd>" << BOOST_THREAD_END_LOG;
  }
  BOOST_CATCH (...)
  {
    //BOOST_THREAD_LOG << "exception thrd>" << BOOST_THREAD_END_LOG;
  }
  BOOST_CATCH_END
  //BOOST_THREAD_LOG << "thrd>" << BOOST_THREAD_END_LOG;
}

int main()
{
  //BOOST_THREAD_LOG << "<main" << BOOST_THREAD_END_LOG;
  const int nrThreads = 20;
  boost::thread* threads[nrThreads];

  for (int i = 0; i < nrThreads; ++i)
    threads[i] = new boost::thread(&thread);

  for (int i = 0; i < nrThreads; ++i)
  {
    threads[i]->join();
    //BOOST_THREAD_LOG << "main" << BOOST_THREAD_END_LOG;
    delete threads[i];
    //BOOST_THREAD_LOG << "main" << BOOST_THREAD_END_LOG;
  }
  //BOOST_THREAD_LOG << "main>" << BOOST_THREAD_END_LOG;
  return 0;
}
