// Copyright (C) 2001-2003
// William E. Kempf
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_THREAD_VERSION 2

#include <boost/thread/thread_only.hpp>
#include <boost/thread/xtime.hpp>
#include <iostream>

struct thread_alarm
{
    thread_alarm(int secs) : m_secs(secs) { }
    void operator()()
    {
        boost::xtime xt;
        boost::xtime_get(&xt, boost::TIME_UTC_);
        xt.sec += m_secs;

        boost::thread::sleep(xt);

        std::cout << "alarm sounded..." << std::endl;
    }

    int m_secs;
};

int main()
{
    int secs = 5;
    std::cout << "setting alarm for 5 seconds..." << std::endl;
    thread_alarm alarm(secs);
    boost::thread thrd(alarm);
    thrd.join();
}
