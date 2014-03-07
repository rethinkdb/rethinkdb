// (C) Copyright 2006-8 Anthony Williams
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_THREAD_VERSION 2

#include <boost/test/unit_test.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_types.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/thread_only.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/condition_variable.hpp>

template<typename Mutex,typename Lock>
struct test_initially_locked
{
    void operator()() const
    {
        Mutex m;
        Lock lock(m);

        BOOST_CHECK(lock);
        BOOST_CHECK(lock.owns_lock());
    }
};

template<typename Mutex,typename Lock>
struct test_initially_unlocked_if_other_thread_has_lock
{
    Mutex m;
    boost::mutex done_mutex;
    bool done;
    bool locked;
    boost::condition_variable done_cond;

    test_initially_unlocked_if_other_thread_has_lock():
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


    void operator()()
    {
        Lock lock(m);

        typedef test_initially_unlocked_if_other_thread_has_lock<Mutex,Lock> this_type;

        boost::thread t(&this_type::locking_thread,this);

        try
        {
            {
                boost::unique_lock<boost::mutex> lk(done_mutex);
                BOOST_CHECK(done_cond.timed_wait(lk,boost::posix_time::seconds(2),
                                                 boost::bind(&this_type::is_done,this)));
                BOOST_CHECK(!locked);
            }

            lock.unlock();
            t.join();
        }
        catch(...)
        {
            lock.unlock();
            t.join();
            throw;
        }
    }
};

template<typename Mutex,typename Lock>
struct test_initially_unlocked_with_try_lock_if_other_thread_has_unique_lock
{
    Mutex m;
    boost::mutex done_mutex;
    bool done;
    bool locked;
    boost::condition_variable done_cond;

    test_initially_unlocked_with_try_lock_if_other_thread_has_unique_lock():
        done(false),locked(false)
    {}

    void locking_thread()
    {
        Lock lock(m,boost::try_to_lock);

        boost::lock_guard<boost::mutex> lk(done_mutex);
        locked=lock.owns_lock();
        done=true;
        done_cond.notify_one();
    }

    bool is_done() const
    {
        return done;
    }


    void operator()()
    {
        boost::unique_lock<Mutex> lock(m);

        typedef test_initially_unlocked_with_try_lock_if_other_thread_has_unique_lock<Mutex,Lock> this_type;

        boost::thread t(&this_type::locking_thread,this);

        try
        {
            {
                boost::unique_lock<boost::mutex> lk(done_mutex);
                BOOST_CHECK(done_cond.timed_wait(lk,boost::posix_time::seconds(2),
                                                 boost::bind(&this_type::is_done,this)));
                BOOST_CHECK(!locked);
            }

            lock.unlock();
            t.join();
        }
        catch(...)
        {
            lock.unlock();
            t.join();
            throw;
        }
    }
};

template<typename Mutex,typename Lock>
struct test_initially_locked_if_other_thread_has_shared_lock
{
    Mutex m;
    boost::mutex done_mutex;
    bool done;
    bool locked;
    boost::condition_variable done_cond;

    test_initially_locked_if_other_thread_has_shared_lock():
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


    void operator()()
    {
        boost::shared_lock<Mutex> lock(m);

        typedef test_initially_locked_if_other_thread_has_shared_lock<Mutex,Lock> this_type;

        boost::thread t(&this_type::locking_thread,this);

        try
        {
            {
                boost::unique_lock<boost::mutex> lk(done_mutex);
                BOOST_CHECK(done_cond.timed_wait(lk,boost::posix_time::seconds(2),
                                                 boost::bind(&this_type::is_done,this)));
                BOOST_CHECK(locked);
            }

            lock.unlock();
            t.join();
        }
        catch(...)
        {
            lock.unlock();
            t.join();
            throw;
        }
    }
};

template<typename Mutex,typename Lock>
struct test_initially_unlocked_with_defer_lock_parameter
{
    void operator()() const
    {
        Mutex m;
        Lock lock(m,boost::defer_lock);

        BOOST_CHECK(!lock);
        BOOST_CHECK(!lock.owns_lock());
    }
};

template<typename Mutex,typename Lock>
struct test_initially_locked_with_adopt_lock_parameter
{
    void operator()() const
    {
        Mutex m;
        m.lock();
        Lock lock(m,boost::adopt_lock);

        BOOST_CHECK(lock);
        BOOST_CHECK(lock.owns_lock());
    }
};
template<typename Mutex,typename Lock>
struct test_initially_lock_shared_with_adopt_lock_parameter
{
    void operator()() const
    {
        Mutex m;
        m.lock_shared();
        Lock lock(m,boost::adopt_lock);

        BOOST_CHECK(lock);
        BOOST_CHECK(lock.owns_lock());
    }
};


template<typename Mutex,typename Lock>
struct test_unlocked_after_unlock_called
{
    void operator()() const
    {
        Mutex m;
        Lock lock(m);
        lock.unlock();
        BOOST_CHECK(!lock);
        BOOST_CHECK(!lock.owns_lock());
    }
};

template<typename Mutex,typename Lock>
struct test_locked_after_lock_called
{
    void operator()() const
    {
        Mutex m;
        Lock lock(m,boost::defer_lock);
        lock.lock();
        BOOST_CHECK(lock);
        BOOST_CHECK(lock.owns_lock());
    }
};

template<typename Mutex,typename Lock>
struct test_locked_after_try_lock_called
{
    void operator()() const
    {
        Mutex m;
        Lock lock(m,boost::defer_lock);
        lock.try_lock();
        BOOST_CHECK(lock);
        BOOST_CHECK(lock.owns_lock());
    }
};

template<typename Mutex,typename Lock>
struct test_unlocked_after_try_lock_if_other_thread_has_lock
{
    Mutex m;
    boost::mutex done_mutex;
    bool done;
    bool locked;
    boost::condition_variable done_cond;

    test_unlocked_after_try_lock_if_other_thread_has_lock():
        done(false),locked(false)
    {}

    void locking_thread()
    {
        Lock lock(m,boost::defer_lock);

        boost::lock_guard<boost::mutex> lk(done_mutex);
        locked=lock.owns_lock();
        done=true;
        done_cond.notify_one();
    }

    bool is_done() const
    {
        return done;
    }


    void operator()()
    {
        Lock lock(m);

        typedef test_unlocked_after_try_lock_if_other_thread_has_lock<Mutex,Lock> this_type;

        boost::thread t(&this_type::locking_thread,this);

        try
        {
            {
                boost::unique_lock<boost::mutex> lk(done_mutex);
                BOOST_CHECK(done_cond.timed_wait(lk,boost::posix_time::seconds(2),
                                                 boost::bind(&this_type::is_done,this)));
                BOOST_CHECK(!locked);
            }

            lock.unlock();
            t.join();
        }
        catch(...)
        {
            lock.unlock();
            t.join();
            throw;
        }
    }
};

template<typename Mutex,typename Lock>
struct test_throws_if_lock_called_when_already_locked
{
    void operator()() const
    {
        Mutex m;
        Lock lock(m);

        BOOST_CHECK_THROW( lock.lock(), boost::lock_error );
    }
};

template<typename Mutex,typename Lock>
struct test_throws_if_try_lock_called_when_already_locked
{
    void operator()() const
    {
        Mutex m;
        Lock lock(m);

        BOOST_CHECK_THROW( lock.try_lock(), boost::lock_error );
    }
};

template<typename Mutex,typename Lock>
struct test_throws_if_unlock_called_when_already_unlocked
{
    void operator()() const
    {
        Mutex m;
        Lock lock(m);
        lock.unlock();

        BOOST_CHECK_THROW( lock.unlock(), boost::lock_error );
    }
};
template<typename Lock>
struct test_default_constructed_has_no_mutex_and_unlocked
{
    void operator()() const
    {
        Lock l;
        BOOST_CHECK(!l.mutex());
        BOOST_CHECK(!l.owns_lock());
    }
};


template<typename Mutex,typename Lock>
struct test_locks_can_be_swapped
{
    void operator()() const
    {
        Mutex m1;
        Mutex m2;
        Mutex m3;

        Lock l1(m1);
        Lock l2(m2);

        BOOST_CHECK_EQUAL(l1.mutex(),&m1);
        BOOST_CHECK_EQUAL(l2.mutex(),&m2);

        l1.swap(l2);

        BOOST_CHECK_EQUAL(l1.mutex(),&m2);
        BOOST_CHECK_EQUAL(l2.mutex(),&m1);

        swap(l1,l2);

        BOOST_CHECK_EQUAL(l1.mutex(),&m1);
        BOOST_CHECK_EQUAL(l2.mutex(),&m2);

#if 0
        l1.swap(Lock(m3));

        BOOST_CHECK_EQUAL(l1.mutex(),&m3);
#endif

    }
};

template<typename Mutex,typename Lock>
void test_lock_is_scoped_lock_concept_for_mutex()
{
    test_default_constructed_has_no_mutex_and_unlocked<Lock>()();
    test_initially_locked<Mutex,Lock>()();
    test_initially_unlocked_with_defer_lock_parameter<Mutex,Lock>()();
    test_initially_locked_with_adopt_lock_parameter<Mutex,Lock>()();
    test_unlocked_after_unlock_called<Mutex,Lock>()();
    test_locked_after_lock_called<Mutex,Lock>()();
    test_throws_if_lock_called_when_already_locked<Mutex,Lock>()();
    test_throws_if_unlock_called_when_already_unlocked<Mutex,Lock>()();
    test_locks_can_be_swapped<Mutex,Lock>()();
    test_locked_after_try_lock_called<Mutex,Lock>()();
    test_throws_if_try_lock_called_when_already_locked<Mutex,Lock>()();
    test_unlocked_after_try_lock_if_other_thread_has_lock<Mutex,Lock>()();
}


BOOST_TEST_CASE_TEMPLATE_FUNCTION(test_scoped_lock_concept,Mutex)
{
    typedef typename Mutex::scoped_lock Lock;

    test_lock_is_scoped_lock_concept_for_mutex<Mutex,Lock>();
}

BOOST_TEST_CASE_TEMPLATE_FUNCTION(test_unique_lock_is_scoped_lock,Mutex)
{
    typedef boost::unique_lock<Mutex> Lock;

    test_lock_is_scoped_lock_concept_for_mutex<Mutex,Lock>();
}

BOOST_TEST_CASE_TEMPLATE_FUNCTION(test_scoped_try_lock_concept,Mutex)
{
    typedef typename Mutex::scoped_try_lock Lock;

    test_default_constructed_has_no_mutex_and_unlocked<Lock>()();
    test_initially_locked<Mutex,Lock>()();
    test_initially_unlocked_if_other_thread_has_lock<Mutex,Lock>()();
    test_initially_unlocked_with_defer_lock_parameter<Mutex,Lock>()();
    test_initially_locked_with_adopt_lock_parameter<Mutex,Lock>()();
    test_unlocked_after_unlock_called<Mutex,Lock>()();
    test_locked_after_lock_called<Mutex,Lock>()();
    test_locked_after_try_lock_called<Mutex,Lock>()();
    test_unlocked_after_try_lock_if_other_thread_has_lock<Mutex,Lock>()();
    test_throws_if_lock_called_when_already_locked<Mutex,Lock>()();
    test_throws_if_try_lock_called_when_already_locked<Mutex,Lock>()();
    test_throws_if_unlock_called_when_already_unlocked<Mutex,Lock>()();
    test_locks_can_be_swapped<Mutex,Lock>()();
}

struct dummy_shared_mutex
{
    bool locked;
    bool shared_locked;
    bool shared_unlocked;
    bool shared_timed_locked_relative;
    bool shared_timed_locked_absolute;
    bool timed_locked_relative;
    bool timed_locked_absolute;

    dummy_shared_mutex():
        locked(false),shared_locked(false),shared_unlocked(false),
        shared_timed_locked_relative(false),
        shared_timed_locked_absolute(false),
        timed_locked_relative(false),
        timed_locked_absolute(false)
    {}

    void lock()
    {
        locked=true;
    }

    void lock_shared()
    {
        shared_locked=true;
    }

    void unlock()
    {}

    void unlock_shared()
    {
        shared_unlocked=true;
    }

    bool timed_lock_shared(boost::system_time)
    {
        shared_timed_locked_absolute=true;
        return false;
    }
    template<typename Duration>
    bool timed_lock_shared(Duration)
    {
        shared_timed_locked_relative=true;
        return false;
    }
    bool timed_lock(boost::system_time)
    {
        timed_locked_absolute=true;
        return false;
    }
    template<typename Duration>
    bool timed_lock(Duration)
    {
        timed_locked_relative=true;
        return false;
    }

};


void test_shared_lock()
{
    typedef boost::shared_mutex Mutex;
    typedef boost::shared_lock<Mutex> Lock;

    test_default_constructed_has_no_mutex_and_unlocked<Lock>()();
    test_initially_locked<Mutex,Lock>()();
    test_initially_unlocked_with_try_lock_if_other_thread_has_unique_lock<Mutex,Lock>()();
    test_initially_locked_if_other_thread_has_shared_lock<Mutex,Lock>()();
    test_initially_unlocked_with_defer_lock_parameter<Mutex,Lock>()();
    test_initially_lock_shared_with_adopt_lock_parameter<Mutex,Lock>()();
    test_unlocked_after_unlock_called<Mutex,Lock>()();
    test_locked_after_lock_called<Mutex,Lock>()();
    test_locked_after_try_lock_called<Mutex,Lock>()();
    test_throws_if_lock_called_when_already_locked<Mutex,Lock>()();
    test_throws_if_try_lock_called_when_already_locked<Mutex,Lock>()();
    test_throws_if_unlock_called_when_already_unlocked<Mutex,Lock>()();
    test_locks_can_be_swapped<Mutex,Lock>()();

    dummy_shared_mutex dummy;
    boost::shared_lock<dummy_shared_mutex> lk(dummy);
    BOOST_CHECK(dummy.shared_locked);
    lk.unlock();
    BOOST_CHECK(dummy.shared_unlocked);
    lk.timed_lock(boost::posix_time::milliseconds(5));
    BOOST_CHECK(dummy.shared_timed_locked_relative);
    lk.timed_lock(boost::get_system_time());
    BOOST_CHECK(dummy.shared_timed_locked_absolute);
}

boost::unit_test::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test::test_suite* test =
        BOOST_TEST_SUITE("Boost.Threads: lock concept test suite");

    typedef boost::mpl::vector<boost::mutex,boost::try_mutex,boost::timed_mutex,
        boost::recursive_mutex,boost::recursive_try_mutex,boost::recursive_timed_mutex> mutex_types_with_scoped_lock;

    test->add(BOOST_TEST_CASE_TEMPLATE(test_scoped_lock_concept,mutex_types_with_scoped_lock));

    typedef boost::mpl::vector<boost::try_mutex,boost::timed_mutex,
        boost::recursive_try_mutex,boost::recursive_timed_mutex> mutex_types_with_scoped_try_lock;

    test->add(BOOST_TEST_CASE_TEMPLATE(test_scoped_try_lock_concept,mutex_types_with_scoped_try_lock));

    typedef boost::mpl::vector<boost::mutex,boost::try_mutex,boost::timed_mutex,
        boost::recursive_mutex,boost::recursive_try_mutex,boost::recursive_timed_mutex,boost::shared_mutex> all_mutex_types;

    test->add(BOOST_TEST_CASE_TEMPLATE(test_unique_lock_is_scoped_lock,all_mutex_types));
    test->add(BOOST_TEST_CASE(&test_shared_lock));

    return test;
}

