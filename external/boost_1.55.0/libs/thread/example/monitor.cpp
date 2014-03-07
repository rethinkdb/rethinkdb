// Copyright (C) 2001-2003
// William E. Kempf
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <vector>
#include <iostream>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/thread_only.hpp>

namespace {
const int ITERS = 100;
boost::mutex io_mutex;
} // namespace

template <typename M>
class buffer_t
{
public:
    typedef boost::unique_lock<M> scoped_lock;

    buffer_t(int n)
        : p(0), c(0), full(0), buf(n)
    {
    }

    void send(int m)
    {
        scoped_lock lk(mutex);
        while (full == buf.size())
            cond.wait(lk);
        buf[p] = m;
        p = (p+1) % buf.size();
        ++full;
        cond.notify_one();
    }
    int receive()
    {
        scoped_lock lk(mutex);
        while (full == 0)
            cond.wait(lk);
        int i = buf[c];
        c = (c+1) % buf.size();
        --full;
        cond.notify_one();
        return i;
    }

    static buffer_t& get_buffer()
    {
        static buffer_t buf(2);
        return buf;
    }

    static void do_sender_thread()
    {
        for (int n = 0; n < ITERS; ++n)
        {
            {
                boost::unique_lock<boost::mutex> lock(io_mutex);
                std::cout << "sending: " << n << std::endl;
            }
            get_buffer().send(n);
        }
    }

    static void do_receiver_thread()
    {
        for (int x=0; x < (ITERS/2); ++x)
        {
            int n = get_buffer().receive();
            {
                boost::unique_lock<boost::mutex> lock(io_mutex);
                std::cout << "received: " << n << std::endl;
            }
        }
    }

private:
    M mutex;
    boost::condition_variable_any cond;
    unsigned int p, c, full;
    std::vector<int> buf;
};

template <typename M>
void do_test(M* dummy=0)
{
    (void)dummy;
    typedef buffer_t<M> buffer_type;
    buffer_type::get_buffer();
    boost::thread thrd1(&buffer_type::do_receiver_thread);
    boost::thread thrd2(&buffer_type::do_receiver_thread);
    boost::thread thrd3(&buffer_type::do_sender_thread);
    thrd1.join();
    thrd2.join();
    thrd3.join();
}

void test_buffer()
{
    do_test<boost::mutex>();
    do_test<boost::recursive_mutex>();
}

int main()
{
    test_buffer();
    return 0;
}
