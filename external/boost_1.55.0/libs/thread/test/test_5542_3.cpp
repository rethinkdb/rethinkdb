// Copyright (C) 2010 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_THREAD_VERSION 2

#include <iostream>
#include <boost/thread/thread_only.hpp>
#include <boost/date_time.hpp>

void workerFunc()
{
   boost::posix_time::seconds workTime(3);

   std::cout << "Worker: running" << std::endl;

   // Pretend to do something useful...
   boost::this_thread::sleep(workTime);

   std::cout << "Worker: finished" << std::endl;
}

int main()
{
    std::cout << "main: startup" << std::endl;

    boost::thread workerThread(workerFunc);

    std::cout << "main: waiting for thread" << std::endl;

    workerThread.join();

    std::cout << "main: done" << std::endl;

    return 0;
}
