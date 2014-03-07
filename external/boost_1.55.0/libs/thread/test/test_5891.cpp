// Copyright (C) 2010 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_THREAD_VERSION 2

#include <iostream>
#include <boost/thread/thread_only.hpp>

using namespace std;
using namespace boost;

  struct X {
    void operator()()
    {
        boost::this_thread::sleep(posix_time::seconds(2));
    }
  };
int main()
{
  X run;
  boost::thread myThread(run);
  boost::this_thread::yield();
  if(myThread.timed_join(posix_time::seconds(5)))
  {
    cout << "thats ok";
    return 0;
  }
  else
  {
    cout << "too late";
    return 1;
  }
}
