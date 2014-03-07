// Copyright (C) 2001-2003
// William E. Kempf
// Copyright (C) 2007-8 Anthony Williams
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(UTIL_INL_WEK01242003)
#define UTIL_INL_WEK01242003

#include <boost/thread/xtime.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/thread.hpp>

#ifndef DEFAULT_EXECUTION_MONITOR_TYPE
#   define DEFAULT_EXECUTION_MONITOR_TYPE execution_monitor::use_condition
#endif

// boostinspect:nounnamed



namespace
{
inline boost::xtime delay(int secs, int msecs=0, int nsecs=0)
{
    const int MILLISECONDS_PER_SECOND = 1000;
    const int NANOSECONDS_PER_SECOND = 1000000000;
    const int NANOSECONDS_PER_MILLISECOND = 1000000;

    boost::xtime xt;
    if (boost::TIME_UTC_ != boost::xtime_get (&xt, boost::TIME_UTC_))
        BOOST_ERROR ("boost::xtime_get != boost::TIME_UTC_");

    nsecs += xt.nsec;
    msecs += nsecs / NANOSECONDS_PER_MILLISECOND;
    secs += msecs / MILLISECONDS_PER_SECOND;
    nsecs += (msecs % MILLISECONDS_PER_SECOND) * NANOSECONDS_PER_MILLISECOND;
    xt.nsec = nsecs % NANOSECONDS_PER_SECOND;
    xt.sec += secs + (nsecs / NANOSECONDS_PER_SECOND);

    return xt;
}

}
namespace boost
{
namespace threads
{
namespace test
{
inline bool in_range(const boost::xtime& xt, int secs=1)
{
    boost::xtime min = delay(-secs);
    boost::xtime max = delay(0);
    return (boost::xtime_cmp(xt, min) >= 0) &&
        (boost::xtime_cmp(xt, max) <= 0);
}
}
}
}


namespace
{
class execution_monitor
{
public:
    enum wait_type { use_sleep_only, use_mutex, use_condition };

    execution_monitor(wait_type type, int secs)
        : done(false), type(type), secs(secs) { }
    void start()
    {
        if (type != use_sleep_only) {
            boost::unique_lock<boost::mutex> lock(mutex); done = false;
        } else {
            done = false;
        }
    }
    void finish()
    {
        if (type != use_sleep_only) {
            boost::unique_lock<boost::mutex> lock(mutex);
            done = true;
            if (type == use_condition)
                cond.notify_one();
        } else {
            done = true;
        }
    }
    bool wait()
    {
        boost::xtime xt = delay(secs);
        if (type != use_condition)
            boost::thread::sleep(xt);
        if (type != use_sleep_only) {
            boost::unique_lock<boost::mutex> lock(mutex);
            while (type == use_condition && !done) {
                if (!cond.timed_wait(lock, xt))
                    break;
            }
            return done;
        }
        return done;
    }

private:
    boost::mutex mutex;
    boost::condition cond;
    bool done;
    wait_type type;
    int secs;
};
}
namespace thread_detail_anon
{
template <typename F>
class indirect_adapter
{
public:
    indirect_adapter(F func, execution_monitor& monitor)
        : func(func), monitor(monitor) { }
    void operator()() const
    {
        try
        {
            boost::thread thrd(func);
            thrd.join();
        }
        catch (...)
        {
            monitor.finish();
            throw;
        }
        monitor.finish();
    }

private:
    F func;
    execution_monitor& monitor;
    void operator=(indirect_adapter&);
};

}
// boostinspect:nounnamed
namespace 
{

template <typename F>
void timed_test(F func, int secs,
    execution_monitor::wait_type type=DEFAULT_EXECUTION_MONITOR_TYPE)
{
    execution_monitor monitor(type, secs);
    thread_detail_anon::indirect_adapter<F> ifunc(func, monitor);
    monitor.start();
    boost::thread thrd(ifunc);
    BOOST_REQUIRE_MESSAGE(monitor.wait(),
        "Timed test didn't complete in time, possible deadlock.");
}

}

namespace thread_detail_anon
{

template <typename F, typename T>
class thread_binder
{
public:
    thread_binder(const F& func, const T& param)
        : func(func), param(param) { }
    void operator()() const { func(param); }

private:
    F func;
    T param;
};

}

// boostinspect:nounnamed
namespace 
{
template <typename F, typename T>
thread_detail_anon::thread_binder<F, T> bind(const F& func, const T& param)
{
    return thread_detail_anon::thread_binder<F, T>(func, param);
}
}

namespace thread_detail_anon
{

template <typename R, typename T>
class thread_member_binder
{
public:
    thread_member_binder(R (T::*func)(), T& param)
        : func(func), param(param) { }
    void operator()() const { (param.*func)(); }

private:
    void operator=(thread_member_binder&);
    
    R (T::*func)();
    T& param;
};

}

// boostinspect:nounnamed
namespace 
{
template <typename R, typename T>
thread_detail_anon::thread_member_binder<R, T> bind(R (T::*func)(), T& param)
{
    return thread_detail_anon::thread_member_binder<R, T>(func, param);
}
} // namespace

#endif
