// (C) Copyright 2008 Anthony Williams
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_THREAD_VERSION 2

#include <boost/test/unit_test.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread_only.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/condition_variable.hpp>

void test_lock_two_uncontended()
{
    boost::mutex m1,m2;

    boost::unique_lock<boost::mutex> l1(m1,boost::defer_lock),
        l2(m2,boost::defer_lock);

    BOOST_CHECK(!l1.owns_lock());
    BOOST_CHECK(!l2.owns_lock());

    boost::lock(l1,l2);

    BOOST_CHECK(l1.owns_lock());
    BOOST_CHECK(l2.owns_lock());
}

struct wait_data
{
    boost::mutex m;
    bool flag;
    boost::condition_variable cond;

    wait_data():
        flag(false)
    {}

    void wait()
    {
        boost::unique_lock<boost::mutex> l(m);
        while(!flag)
        {
            cond.wait(l);
        }
    }

    template<typename Duration>
    bool timed_wait(Duration d)
    {
        boost::system_time const target=boost::get_system_time()+d;

        boost::unique_lock<boost::mutex> l(m);
        while(!flag)
        {
            if(!cond.timed_wait(l,target))
            {
                return flag;
            }
        }
        return true;
    }

    void signal()
    {
        boost::unique_lock<boost::mutex> l(m);
        flag=true;
        cond.notify_all();
    }
};


void lock_mutexes_slowly(boost::mutex* m1,boost::mutex* m2,wait_data* locked,wait_data* quit)
{
    boost::lock_guard<boost::mutex> l1(*m1);
    boost::this_thread::sleep(boost::posix_time::milliseconds(500));
    boost::lock_guard<boost::mutex> l2(*m2);
    locked->signal();
    quit->wait();
}

void lock_pair(boost::mutex* m1,boost::mutex* m2)
{
    boost::lock(*m1,*m2);
    boost::unique_lock<boost::mutex> l1(*m1,boost::adopt_lock),
        l2(*m2,boost::adopt_lock);
}

void test_lock_two_other_thread_locks_in_order()
{
    boost::mutex m1,m2;
    wait_data locked;
    wait_data release;

    boost::thread t(lock_mutexes_slowly,&m1,&m2,&locked,&release);
    boost::this_thread::sleep(boost::posix_time::milliseconds(10));

    boost::thread t2(lock_pair,&m1,&m2);
    BOOST_CHECK(locked.timed_wait(boost::posix_time::seconds(1)));

    release.signal();

    BOOST_CHECK(t2.timed_join(boost::posix_time::seconds(1)));

    t.join();
}

void test_lock_two_other_thread_locks_in_opposite_order()
{
    boost::mutex m1,m2;
    wait_data locked;
    wait_data release;

    boost::thread t(lock_mutexes_slowly,&m1,&m2,&locked,&release);
    boost::this_thread::sleep(boost::posix_time::milliseconds(10));

    boost::thread t2(lock_pair,&m2,&m1);
    BOOST_CHECK(locked.timed_wait(boost::posix_time::seconds(1)));

    release.signal();

    BOOST_CHECK(t2.timed_join(boost::posix_time::seconds(1)));

    t.join();
}

void test_lock_five_uncontended()
{
    boost::mutex m1,m2,m3,m4,m5;

    boost::unique_lock<boost::mutex> l1(m1,boost::defer_lock),
        l2(m2,boost::defer_lock),
        l3(m3,boost::defer_lock),
        l4(m4,boost::defer_lock),
        l5(m5,boost::defer_lock);

    BOOST_CHECK(!l1.owns_lock());
    BOOST_CHECK(!l2.owns_lock());
    BOOST_CHECK(!l3.owns_lock());
    BOOST_CHECK(!l4.owns_lock());
    BOOST_CHECK(!l5.owns_lock());

    boost::lock(l1,l2,l3,l4,l5);

    BOOST_CHECK(l1.owns_lock());
    BOOST_CHECK(l2.owns_lock());
    BOOST_CHECK(l3.owns_lock());
    BOOST_CHECK(l4.owns_lock());
    BOOST_CHECK(l5.owns_lock());
}

void lock_five_mutexes_slowly(boost::mutex* m1,boost::mutex* m2,boost::mutex* m3,boost::mutex* m4,boost::mutex* m5,
                              wait_data* locked,wait_data* quit)
{
    boost::lock_guard<boost::mutex> l1(*m1);
    boost::this_thread::sleep(boost::posix_time::milliseconds(500));
    boost::lock_guard<boost::mutex> l2(*m2);
    boost::this_thread::sleep(boost::posix_time::milliseconds(500));
    boost::lock_guard<boost::mutex> l3(*m3);
    boost::this_thread::sleep(boost::posix_time::milliseconds(500));
    boost::lock_guard<boost::mutex> l4(*m4);
    boost::this_thread::sleep(boost::posix_time::milliseconds(500));
    boost::lock_guard<boost::mutex> l5(*m5);
    locked->signal();
    quit->wait();
}

void lock_five(boost::mutex* m1,boost::mutex* m2,boost::mutex* m3,boost::mutex* m4,boost::mutex* m5)
{
    boost::lock(*m1,*m2,*m3,*m4,*m5);
    m1->unlock();
    m2->unlock();
    m3->unlock();
    m4->unlock();
    m5->unlock();
}

void test_lock_five_other_thread_locks_in_order()
{
    boost::mutex m1,m2,m3,m4,m5;
    wait_data locked;
    wait_data release;

    boost::thread t(lock_five_mutexes_slowly,&m1,&m2,&m3,&m4,&m5,&locked,&release);
    boost::this_thread::sleep(boost::posix_time::milliseconds(10));

    boost::thread t2(lock_five,&m1,&m2,&m3,&m4,&m5);
    BOOST_CHECK(locked.timed_wait(boost::posix_time::seconds(3)));

    release.signal();

    BOOST_CHECK(t2.timed_join(boost::posix_time::seconds(3)));

    t.join();
}

void test_lock_five_other_thread_locks_in_different_order()
{
    boost::mutex m1,m2,m3,m4,m5;
    wait_data locked;
    wait_data release;

    boost::thread t(lock_five_mutexes_slowly,&m1,&m2,&m3,&m4,&m5,&locked,&release);
    boost::this_thread::sleep(boost::posix_time::milliseconds(10));

    boost::thread t2(lock_five,&m5,&m1,&m4,&m2,&m3);
    BOOST_CHECK(locked.timed_wait(boost::posix_time::seconds(3)));

    release.signal();

    BOOST_CHECK(t2.timed_join(boost::posix_time::seconds(3)));

    t.join();
}

void lock_n(boost::mutex* mutexes,unsigned count)
{
    boost::lock(mutexes,mutexes+count);
    for(unsigned i=0;i<count;++i)
    {
        mutexes[i].unlock();
    }
}


void test_lock_ten_other_thread_locks_in_different_order()
{
    unsigned const num_mutexes=10;

    boost::mutex mutexes[num_mutexes];
    wait_data locked;
    wait_data release;

    boost::thread t(lock_five_mutexes_slowly,&mutexes[6],&mutexes[3],&mutexes[8],&mutexes[0],&mutexes[2],&locked,&release);
    boost::this_thread::sleep(boost::posix_time::milliseconds(10));

    boost::thread t2(lock_n,mutexes,num_mutexes);
    BOOST_CHECK(locked.timed_wait(boost::posix_time::seconds(3)));

    release.signal();

    BOOST_CHECK(t2.timed_join(boost::posix_time::seconds(3)));

    t.join();
}

struct dummy_mutex
{
    bool is_locked;

    dummy_mutex():
        is_locked(false)
    {}

    void lock()
    {
        is_locked=true;
    }

    bool try_lock()
    {
        if(is_locked)
        {
            return false;
        }
        is_locked=true;
        return true;
    }

    void unlock()
    {
        is_locked=false;
    }
};

namespace boost
{
    template<>
    struct is_mutex_type<dummy_mutex>
    {
        BOOST_STATIC_CONSTANT(bool, value = true);
    };
}



void test_lock_five_in_range()
{
    unsigned const num_mutexes=5;
    dummy_mutex mutexes[num_mutexes];

    boost::lock(mutexes,mutexes+num_mutexes);

    for(unsigned i=0;i<num_mutexes;++i)
    {
        BOOST_CHECK(mutexes[i].is_locked);
    }
}

class dummy_iterator:
    public std::iterator<std::forward_iterator_tag,
                         dummy_mutex>
{
private:
    dummy_mutex* p;
public:
    explicit dummy_iterator(dummy_mutex* p_):
        p(p_)
    {}

    bool operator==(dummy_iterator const& other) const
    {
        return p==other.p;
    }

    bool operator!=(dummy_iterator const& other) const
    {
        return p!=other.p;
    }

    bool operator<(dummy_iterator const& other) const
    {
        return p<other.p;
    }

    dummy_mutex& operator*() const
    {
        return *p;
    }

    dummy_mutex* operator->() const
    {
        return p;
    }

    dummy_iterator operator++(int)
    {
        dummy_iterator temp(*this);
        ++p;
        return temp;
    }

    dummy_iterator& operator++()
    {
        ++p;
        return *this;
    }

};


void test_lock_five_in_range_custom_iterator()
{
    unsigned const num_mutexes=5;
    dummy_mutex mutexes[num_mutexes];

    boost::lock(dummy_iterator(mutexes),dummy_iterator(mutexes+num_mutexes));

    for(unsigned i=0;i<num_mutexes;++i)
    {
        BOOST_CHECK(mutexes[i].is_locked);
    }
}

class dummy_mutex2:
    public dummy_mutex
{};


void test_lock_ten_in_range_inherited_mutex()
{
    unsigned const num_mutexes=10;
    dummy_mutex2 mutexes[num_mutexes];

    boost::lock(mutexes,mutexes+num_mutexes);

    for(unsigned i=0;i<num_mutexes;++i)
    {
        BOOST_CHECK(mutexes[i].is_locked);
    }
}

void test_try_lock_two_uncontended()
{
    dummy_mutex m1,m2;

    int const res=boost::try_lock(m1,m2);

    BOOST_CHECK(res==-1);
    BOOST_CHECK(m1.is_locked);
    BOOST_CHECK(m2.is_locked);
}
void test_try_lock_two_first_locked()
{
    dummy_mutex m1,m2;
    m1.lock();

    boost::unique_lock<dummy_mutex> l1(m1,boost::defer_lock),
        l2(m2,boost::defer_lock);

    int const res=boost::try_lock(l1,l2);

    BOOST_CHECK(res==0);
    BOOST_CHECK(m1.is_locked);
    BOOST_CHECK(!m2.is_locked);
    BOOST_CHECK(!l1.owns_lock());
    BOOST_CHECK(!l2.owns_lock());
}
void test_try_lock_two_second_locked()
{
    dummy_mutex m1,m2;
    m2.lock();

    boost::unique_lock<dummy_mutex> l1(m1,boost::defer_lock),
        l2(m2,boost::defer_lock);

    int const res=boost::try_lock(l1,l2);

    BOOST_CHECK(res==1);
    BOOST_CHECK(!m1.is_locked);
    BOOST_CHECK(m2.is_locked);
    BOOST_CHECK(!l1.owns_lock());
    BOOST_CHECK(!l2.owns_lock());
}

void test_try_lock_three()
{
    int const num_mutexes=3;

    for(int i=-1;i<num_mutexes;++i)
    {
        dummy_mutex mutexes[num_mutexes];

        if(i>=0)
        {
            mutexes[i].lock();
        }
        boost::unique_lock<dummy_mutex> l1(mutexes[0],boost::defer_lock),
            l2(mutexes[1],boost::defer_lock),
            l3(mutexes[2],boost::defer_lock);

        int const res=boost::try_lock(l1,l2,l3);

        BOOST_CHECK(res==i);
        for(int j=0;j<num_mutexes;++j)
        {
            if((i==j) || (i==-1))
            {
                BOOST_CHECK(mutexes[j].is_locked);
            }
            else
            {
                BOOST_CHECK(!mutexes[j].is_locked);
            }
        }
        if(i==-1)
        {
            BOOST_CHECK(l1.owns_lock());
            BOOST_CHECK(l2.owns_lock());
            BOOST_CHECK(l3.owns_lock());
        }
        else
        {
            BOOST_CHECK(!l1.owns_lock());
            BOOST_CHECK(!l2.owns_lock());
            BOOST_CHECK(!l3.owns_lock());
        }
    }
}

void test_try_lock_four()
{
    int const num_mutexes=4;

    for(int i=-1;i<num_mutexes;++i)
    {
        dummy_mutex mutexes[num_mutexes];

        if(i>=0)
        {
            mutexes[i].lock();
        }
        boost::unique_lock<dummy_mutex> l1(mutexes[0],boost::defer_lock),
            l2(mutexes[1],boost::defer_lock),
            l3(mutexes[2],boost::defer_lock),
            l4(mutexes[3],boost::defer_lock);

        int const res=boost::try_lock(l1,l2,l3,l4);

        BOOST_CHECK(res==i);
        for(int j=0;j<num_mutexes;++j)
        {
            if((i==j) || (i==-1))
            {
                BOOST_CHECK(mutexes[j].is_locked);
            }
            else
            {
                BOOST_CHECK(!mutexes[j].is_locked);
            }
        }
        if(i==-1)
        {
            BOOST_CHECK(l1.owns_lock());
            BOOST_CHECK(l2.owns_lock());
            BOOST_CHECK(l3.owns_lock());
            BOOST_CHECK(l4.owns_lock());
        }
        else
        {
            BOOST_CHECK(!l1.owns_lock());
            BOOST_CHECK(!l2.owns_lock());
            BOOST_CHECK(!l3.owns_lock());
            BOOST_CHECK(!l4.owns_lock());
        }
    }
}

void test_try_lock_five()
{
    int const num_mutexes=5;

    for(int i=-1;i<num_mutexes;++i)
    {
        dummy_mutex mutexes[num_mutexes];

        if(i>=0)
        {
            mutexes[i].lock();
        }
        boost::unique_lock<dummy_mutex> l1(mutexes[0],boost::defer_lock),
            l2(mutexes[1],boost::defer_lock),
            l3(mutexes[2],boost::defer_lock),
            l4(mutexes[3],boost::defer_lock),
            l5(mutexes[4],boost::defer_lock);

        int const res=boost::try_lock(l1,l2,l3,l4,l5);

        BOOST_CHECK(res==i);
        for(int j=0;j<num_mutexes;++j)
        {
            if((i==j) || (i==-1))
            {
                BOOST_CHECK(mutexes[j].is_locked);
            }
            else
            {
                BOOST_CHECK(!mutexes[j].is_locked);
            }
        }
        if(i==-1)
        {
            BOOST_CHECK(l1.owns_lock());
            BOOST_CHECK(l2.owns_lock());
            BOOST_CHECK(l3.owns_lock());
            BOOST_CHECK(l4.owns_lock());
            BOOST_CHECK(l5.owns_lock());
        }
        else
        {
            BOOST_CHECK(!l1.owns_lock());
            BOOST_CHECK(!l2.owns_lock());
            BOOST_CHECK(!l3.owns_lock());
            BOOST_CHECK(!l4.owns_lock());
            BOOST_CHECK(!l5.owns_lock());
        }
    }
}



boost::unit_test::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test::test_suite* test =
        BOOST_TEST_SUITE("Boost.Threads: generic locks test suite");

    test->add(BOOST_TEST_CASE(&test_lock_two_uncontended));
    test->add(BOOST_TEST_CASE(&test_lock_two_other_thread_locks_in_order));
    test->add(BOOST_TEST_CASE(&test_lock_two_other_thread_locks_in_opposite_order));
    test->add(BOOST_TEST_CASE(&test_lock_five_uncontended));
    test->add(BOOST_TEST_CASE(&test_lock_five_other_thread_locks_in_order));
    test->add(BOOST_TEST_CASE(&test_lock_five_other_thread_locks_in_different_order));
    test->add(BOOST_TEST_CASE(&test_lock_five_in_range));
    test->add(BOOST_TEST_CASE(&test_lock_five_in_range_custom_iterator));
    test->add(BOOST_TEST_CASE(&test_lock_ten_in_range_inherited_mutex));
    test->add(BOOST_TEST_CASE(&test_lock_ten_other_thread_locks_in_different_order));
    test->add(BOOST_TEST_CASE(&test_try_lock_two_uncontended));
    test->add(BOOST_TEST_CASE(&test_try_lock_two_first_locked));
    test->add(BOOST_TEST_CASE(&test_try_lock_two_second_locked));
    test->add(BOOST_TEST_CASE(&test_try_lock_three));
    test->add(BOOST_TEST_CASE(&test_try_lock_four));
    test->add(BOOST_TEST_CASE(&test_try_lock_five));

    return test;
}


