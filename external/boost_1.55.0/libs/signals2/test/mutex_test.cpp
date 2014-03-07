// Copyright (C) 2001-2003
// William E. Kempf
//
// Copyright Frank Mori Hess 2009
//
// Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// This is a simplified/modified version of libs/thread/test/test_mutex.cpp
// added to test boost::signals2::mutex.
// For more information, see http://www.boost.org

// note boost/test/minimal.hpp can cause windows.h to get included, which
// can screw up our checks of _WIN32_WINNT if it is included
// after boost/signals2/mutex.hpp.  Frank Hess 2009-03-07.
#include <boost/test/minimal.hpp>

#include <boost/bind.hpp>
#include <boost/signals2/dummy_mutex.hpp>
#include <boost/signals2/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/thread_time.hpp>
#include <boost/thread/condition.hpp>

class execution_monitor
{
public:
    enum wait_type { use_sleep_only, use_mutex, use_condition };

    execution_monitor(wait_type type, int secs)
        : done(false), m_type(type), m_secs(secs) { }
    void start()
    {
        if (m_type != use_sleep_only) {
            boost::mutex::scoped_lock lock(mutex); done = false;
        } else {
            done = false;
        }
    }
    void finish()
    {
        if (m_type != use_sleep_only) {
            boost::mutex::scoped_lock lock(mutex);
            done = true;
            if (m_type == use_condition)
                cond.notify_one();
        } else {
            done = true;
        }
    }
    bool wait()
    {
        boost::posix_time::time_duration timeout = boost::posix_time::seconds(m_secs);
        if (m_type != use_condition)
            boost::this_thread::sleep(timeout);
        if (m_type != use_sleep_only) {
            boost::mutex::scoped_lock lock(mutex);
            while (m_type == use_condition && !done) {
                if (!cond.timed_wait(lock, timeout))
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
    wait_type m_type;
    int m_secs;
};

template <typename F>
class indirect_adapter
{
public:
    indirect_adapter(F func, execution_monitor& monitor)
        : m_func(func), m_monitor(monitor) { }
    void operator()() const
    {
        try
        {
            boost::thread thrd(m_func);
            thrd.join();
        }
        catch (...)
        {
            m_monitor.finish();
            throw;
        }
        m_monitor.finish();
    }

private:
    F m_func;
    execution_monitor& m_monitor;
    void operator=(indirect_adapter&);
};

template <typename F>
void timed_test(F func, int secs,
    execution_monitor::wait_type type = execution_monitor::use_sleep_only)
{
    execution_monitor monitor(type, secs);
    indirect_adapter<F> ifunc(func, monitor);
    monitor.start();
    boost::thread thrd(ifunc);
    BOOST_REQUIRE(monitor.wait()); // Timed test didn't complete in time, possible deadlock
}

template <typename M>
struct test_lock
{
    typedef M mutex_type;
    typedef typename boost::unique_lock<M> lock_type;

    void operator()()
    {
        mutex_type mutex;
        boost::condition condition;

        // Test the lock's constructors.
        {
            lock_type lock(mutex, boost::defer_lock);
            BOOST_CHECK(!lock);
        }
        lock_type lock(mutex);
        BOOST_CHECK(lock ? true : false);

        // Construct a fast time out.
        boost::posix_time::time_duration timeout = boost::posix_time::milliseconds(100);

        // Test the lock and the mutex with condition variables.
        // No one is going to notify this condition variable.  We expect to
        // time out.
        BOOST_CHECK(!condition.timed_wait(lock, timeout));
        BOOST_CHECK(lock ? true : false);

        // Test the lock and unlock methods.
        lock.unlock();
        BOOST_CHECK(!lock);
        lock.lock();
        BOOST_CHECK(lock ? true : false);
    }
};

template <typename M>
struct test_trylock
{
    typedef M mutex_type;
    typedef typename boost::unique_lock<M> lock_type;

    void operator()()
    {
        mutex_type mutex;
        boost::condition condition;

        // Test the lock's constructors.
        {
            lock_type lock(mutex, boost::try_to_lock);
            BOOST_CHECK(lock ? true : false);
        }
        {
            lock_type lock(mutex, boost::defer_lock);
            BOOST_CHECK(!lock);
        }
        lock_type lock(mutex, boost::try_to_lock);
        BOOST_CHECK(lock ? true : false);

        // Construct a fast time out.
        boost::posix_time::time_duration timeout = boost::posix_time::milliseconds(100);

        // Test the lock and the mutex with condition variables.
        // No one is going to notify this condition variable.  We expect to
        // time out.
        BOOST_CHECK(!condition.timed_wait(lock, timeout));
        BOOST_CHECK(lock ? true : false);

        // Test the lock, unlock and trylock methods.
        lock.unlock();
        BOOST_CHECK(!lock);
        lock.lock();
        BOOST_CHECK(lock ? true : false);
        lock.unlock();
        BOOST_CHECK(!lock);
        BOOST_CHECK(lock.try_lock());
        BOOST_CHECK(lock ? true : false);
    }
};

template<typename Mutex>
struct test_lock_exclusion
{
    typedef boost::unique_lock<Mutex> Lock;

    Mutex m;
    boost::mutex done_mutex;
    bool done;
    bool locked;
    boost::condition_variable done_cond;

    test_lock_exclusion():
        done(false),locked(false)
    {}

    void locking_thread()
    {
        Lock lock(m);

        boost::lock_guard<boost::mutex> lk(done_mutex);
        locked=lock.owns_lock();
        done=true;
        done_cond.notify_one();
    }

    bool is_done() const
    {
        return done;
    }

    typedef test_lock_exclusion<Mutex> this_type;

    void do_test(void (this_type::*test_func)())
    {
        Lock lock(m);

        locked=false;
        done=false;

        boost::thread t(test_func,this);

        try
        {
            {
                boost::mutex::scoped_lock lk(done_mutex);
                BOOST_CHECK(!done_cond.timed_wait(lk, boost::posix_time::seconds(1),
                                                 boost::bind(&this_type::is_done,this)));
            }
            lock.unlock();
            {
                boost::mutex::scoped_lock lk(done_mutex);
                BOOST_CHECK(done_cond.timed_wait(lk, boost::posix_time::seconds(1),
                                                 boost::bind(&this_type::is_done,this)));
            }
            t.join();
            BOOST_CHECK(locked);
        }
        catch(...)
        {
            lock.unlock();
            t.join();
            throw;
        }
    }


    void operator()()
    {
        do_test(&this_type::locking_thread);
    }
};


void do_test_mutex()
{
    test_lock<boost::signals2::mutex>()();
// try_lock not supported on old versions of windows
#if !defined(BOOST_HAS_WINTHREADS) || (defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0400))
    test_trylock<boost::signals2::mutex>()();
#endif
    test_lock_exclusion<boost::signals2::mutex>()();
}

void test_mutex()
{
    timed_test(&do_test_mutex, 3);
}

void do_test_dummy_mutex()
{
    test_lock<boost::signals2::dummy_mutex>()();
    test_trylock<boost::signals2::dummy_mutex>()();
}

void test_dummy_mutex()
{
    timed_test(&do_test_dummy_mutex, 2);
}

int test_main(int, char*[])
{
    test_mutex();
    test_dummy_mutex();

    return 0;
}
