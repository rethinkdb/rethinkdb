#ifndef CONDITION_TEST_COMMON_HPP
#define CONDITION_TEST_COMMON_HPP
// Copyright (C) 2007 Anthony Williams
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread_time.hpp>

unsigned const timeout_seconds=5;

struct wait_for_flag
{
    boost::mutex mutex;
    boost::condition_variable cond_var;
    bool flag;
    unsigned woken;

    wait_for_flag():
        flag(false),woken(0)
    {}

    struct check_flag
    {
        bool const& flag;

        check_flag(bool const& flag_):
            flag(flag_)
        {}

        bool operator()() const
        {
            return flag;
        }
    private:
        void operator=(check_flag&);
    };


    void wait_without_predicate()
    {
        boost::unique_lock<boost::mutex> lock(mutex);
        while(!flag)
        {
            cond_var.wait(lock);
        }
        ++woken;
    }

    void wait_with_predicate()
    {
        boost::unique_lock<boost::mutex> lock(mutex);
        cond_var.wait(lock,check_flag(flag));
        if(flag)
        {
            ++woken;
        }
    }

    void timed_wait_without_predicate()
    {
        boost::system_time const timeout=boost::get_system_time()+boost::posix_time::seconds(timeout_seconds);

        boost::unique_lock<boost::mutex> lock(mutex);
        while(!flag)
        {
            if(!cond_var.timed_wait(lock,timeout))
            {
                return;
            }
        }
        ++woken;
    }

    void timed_wait_with_predicate()
    {
        boost::system_time const timeout=boost::get_system_time()+boost::posix_time::seconds(timeout_seconds);
        boost::unique_lock<boost::mutex> lock(mutex);
        if(cond_var.timed_wait(lock,timeout,check_flag(flag)) && flag)
        {
            ++woken;
        }
    }
    void relative_timed_wait_with_predicate()
    {
        boost::unique_lock<boost::mutex> lock(mutex);
        if(cond_var.timed_wait(lock,boost::posix_time::seconds(timeout_seconds),check_flag(flag)) && flag)
        {
            ++woken;
        }
    }
};


#endif
