// Copyright (C) 2001-2003
// William E. Kempf
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_THREAD_VERSION 2

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/thread_only.hpp>
#include <boost/thread/xtime.hpp>
#include <iostream>
#include <time.h>

namespace
{
boost::mutex iomx;
} // namespace

class canteen
{
public:
    canteen() : m_chickens(0) { }

    void get(int id)
    {
        boost::unique_lock<boost::mutex> lock(m_mutex);
        while (m_chickens == 0)
        {
            {
                boost::unique_lock<boost::mutex> lk(iomx);
                std::cout << "(" << clock() << ") Phil" << id <<
                    ": wot, no chickens?  I'll WAIT ..." << std::endl;
            }
            m_condition.wait(lock);
        }
        {
            boost::unique_lock<boost::mutex> lk(iomx);
            std::cout << "(" << clock() << ") Phil" << id <<
                ": those chickens look good ... one please ..." << std::endl;
        }
        m_chickens--;
    }
    void put(int value)
    {
        boost::unique_lock<boost::mutex> lock(m_mutex);
        {
            boost::unique_lock<boost::mutex> lk(iomx);
            std::cout << "(" << clock()
                      << ") Chef: ouch ... make room ... this dish is "
                      << "very hot ..." << std::endl;
        }
        boost::xtime xt;
        boost::xtime_get(&xt, boost::TIME_UTC_);
        xt.sec += 3;
        boost::thread::sleep(xt);
        m_chickens += value;
        {
            boost::unique_lock<boost::mutex> lk(iomx);
            std::cout << "(" << clock() <<
                ") Chef: more chickens ... " << m_chickens <<
                " now available ... NOTIFYING ..." << std::endl;
        }
        m_condition.notify_all();
    }

private:
    boost::mutex m_mutex;
    boost::condition m_condition;
    int m_chickens;
};

canteen g_canteen;

void chef()
{
    const int chickens = 4;
    {
        boost::unique_lock<boost::mutex> lock(iomx);
        std::cout << "(" << clock() << ") Chef: starting ..." << std::endl;
    }
    for (;;)
    {
        {
            boost::unique_lock<boost::mutex> lock(iomx);
            std::cout << "(" << clock() << ") Chef: cooking ..." << std::endl;
        }
        boost::xtime xt;
        boost::xtime_get(&xt, boost::TIME_UTC_);
        xt.sec += 2;
        boost::thread::sleep(xt);
        {
            boost::unique_lock<boost::mutex> lock(iomx);
            std::cout << "(" << clock() << ") Chef: " << chickens
                      << " chickens, ready-to-go ..." << std::endl;
        }
        g_canteen.put(chickens);
    }
}

struct phil
{
    phil(int id) : m_id(id) { }
    void run() {
        {
            boost::unique_lock<boost::mutex> lock(iomx);
            std::cout << "(" << clock() << ") Phil" << m_id
                      << ": starting ..." << std::endl;
        }
        for (;;)
        {
            if (m_id > 0)
            {
                boost::xtime xt;
                boost::xtime_get(&xt, boost::TIME_UTC_);
                xt.sec += 3;
                boost::thread::sleep(xt);
            }
            {
                boost::unique_lock<boost::mutex> lk(iomx);
                std::cout << "(" << clock() << ") Phil" << m_id
                          << ": gotta eat ..." << std::endl;
            }
            g_canteen.get(m_id);
            {
                boost::unique_lock<boost::mutex> lk(iomx);
                std::cout << "(" << clock() << ") Phil" << m_id
                          << ": mmm ... that's good ..." << std::endl;
            }
        }
    }
    static void do_thread(void* param) {
        static_cast<phil*>(param)->run();
    }

    int m_id;
};

struct thread_adapt
{
    thread_adapt(void (*func)(void*), void* param)
        : _func(func), _param(param)
    {
    }
    int operator()() const
    {
        _func(_param);
        return 0;
    }

    void (*_func)(void*);
    void* _param;
};

class thread_adapter
{
public:
    thread_adapter(void (*func)(void*), void* param)
        : _func(func), _param(param)
    {
    }
    void operator()() const { _func(_param); }
private:
    void (*_func)(void*);
    void* _param;
};

int main()
{
    boost::thread thrd_chef(&chef);
    phil p[] = { phil(0), phil(1), phil(2), phil(3), phil(4) };
    boost::thread thrd_phil0(thread_adapter(&phil::do_thread, &p[0]));
    boost::thread thrd_phil1(thread_adapter(&phil::do_thread, &p[1]));
    boost::thread thrd_phil2(thread_adapter(&phil::do_thread, &p[2]));
    boost::thread thrd_phil3(thread_adapter(&phil::do_thread, &p[3]));
    boost::thread thrd_phil4(thread_adapter(&phil::do_thread, &p[4]));

    thrd_chef.join();
    thrd_phil0.join();
    thrd_phil1.join();
    thrd_phil2.join();
    thrd_phil3.join();
    thrd_phil4.join();

    return 0;
}
