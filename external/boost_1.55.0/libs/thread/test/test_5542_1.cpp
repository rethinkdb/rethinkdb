// Copyright (C) 2010 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_THREAD_VERSION 2

#include <iostream>
#include <boost/thread/thread_only.hpp>

class Worker
{
public:

    Worker()
    {
        // the thread is not-a-thread until we call start()
    }

    void start(int N)
    {
//      std::cout << "start\n";
        m_Thread = boost::thread(&Worker::processQueue, this, N);
//        std::cout << "started\n";
    }

    void join()
    {
        m_Thread.join();
    }

    void processQueue(unsigned N)
    {
        float ms = N * 1e3;
        boost::posix_time::milliseconds workTime(ms);

//        std::cout << "Worker: started, will work for "
//                  << ms << "ms"
//                  << std::endl;

        // We're busy, honest!
        boost::this_thread::sleep(workTime);

//        std::cout << "Worker: completed" << std::endl;
    }

private:

    boost::thread m_Thread;
};

int main()
{
//    std::cout << "main: startup" << std::endl;

    Worker worker;

//    std::cout << "main: create worker" << std::endl;

    worker.start(3);

//    std::cout << "main: waiting for thread" << std::endl;

    worker.join();

//    std::cout << "main: done" << std::endl;

    return 0;
}
