// Copyright (C) 2010 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_THREAD_VERSION 2

#include <boost/thread/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <list>
#include <iostream>

#if defined BOOST_THREAD_USES_CHRONO

using namespace std;

boost::recursive_mutex theMutex;

typedef std::list<boost::condition*> Conditions;
Conditions theConditions;

void ThreadFuncWaiter()
{
  boost::condition con1;
  //for(; ; )
  for (int j = 0; j < 10; j++)
  {
    {
      boost::unique_lock<boost::recursive_mutex> lockMtx(theMutex);
      theConditions.push_back(&con1);

      cout << "Added " << boost::this_thread::get_id() << " " << &con1 << endl;
      if (con1.timed_wait(lockMtx, boost::posix_time::time_duration(0, 0, 50)))
      {
        cout << "Woke Up " << boost::this_thread::get_id() << " " << &con1 << endl;
      }
      else
      {
        cout << "*****Timed Out " << boost::this_thread::get_id() << " " << &con1 << endl;
        exit(13);
      }

      theConditions.remove(&con1);
      cout << "Removed " << boost::this_thread::get_id() << " " << &con1 << endl;
      cout << "Waiter " << j << endl;

    }
    //Sleep(2000);
    boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
  }
}

void ThreadFuncNotifier()
{
  for (int j = 0; j < 70; j++)
  {
    {
      boost::unique_lock<boost::recursive_mutex> lockMtx(theMutex);
      cout << "<Notifier " << j << endl;

      unsigned int i = 0;
      for (Conditions::iterator it = theConditions.begin(); it != theConditions.end() && i < 2; ++it)
      {
        (*it)->notify_one();
        //WORKAROUND_ lockMtx.unlock();
        //WORKAROUND_ boost::this_thread::sleep_for(boost::chrono::milliseconds(50));
        cout << "Notified One " << theConditions.size() << " " << (*it) << endl;
        ++i;
        //WORKAROUND_ lockMtx.lock();
      }

      cout << "Notifier> " << j << endl;
    }
    boost::this_thread::sleep_for(boost::chrono::milliseconds(50));
  }
}

int main()
{
  boost::thread_group tg;
  for (int i = 0; i < 12; ++i)
  {
    tg.create_thread(ThreadFuncWaiter);
  }

  tg.create_thread(ThreadFuncNotifier);

  tg.join_all();

  return 0;
}


#else
#error "Test not applicable: BOOST_THREAD_USES_CHRONO not defined for this platform as not supported"
#endif
