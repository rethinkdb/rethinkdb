// Copyright (C) 2012 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_THREAD_PROVIDES_SHARED_MUTEX_UPWARDS_CONVERSIONS
#define BOOST_THREAD_PROVIDES_EXPLICIT_LOCK_CONVERSION
#define BOOST_THREAD_PROVIDES_GENERIC_SHARED_MUTEX_ON_WIN

#include <iostream>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/lock_algorithms.hpp>
#include <boost/thread/thread_only.hpp>
#include <vector>

#if defined BOOST_THREAD_USES_CHRONO
#include <boost/chrono/chrono_io.hpp>


enum {reading, writing};
int state = reading;

#if 1

boost::mutex&
cout_mut()
{
    static boost::mutex m;
    return m;
}

void
print(const char* tag, unsigned count, char ch)
{
    boost::lock_guard<boost::mutex> _(cout_mut());
    std::cout << tag << count << ch;
}

#elif 0

boost::recursive_mutex&
cout_mut()
{
    static boost::recursive_mutex m;
    return m;
}

void print() {}

template <class A0, class ...Args>
void
print(const A0& a0, const Args& ...args)
{
    boost::lock_guard<boost::recursive_mutex> _(cout_mut());
    std::cout << a0;
    print(args...);
}

#else

template <class A0, class A1, class A2>
void
print(const A0&, const A1& a1, const A2&)
{
    assert(a1 > 10000);
}

#endif

namespace S
{

boost::shared_mutex mut;

void reader()
{
    typedef boost::chrono::steady_clock Clock;
    unsigned count = 0;
    Clock::time_point until = Clock::now() + boost::chrono::seconds(3);
    while (Clock::now() < until)
    {
        mut.lock_shared();
        assert(state == reading);
        ++count;
        mut.unlock_shared();
    }
    print("reader = ", count, '\n');
}

void writer()
{
    typedef boost::chrono::steady_clock Clock;
    unsigned count = 0;
    Clock::time_point until = Clock::now() + boost::chrono::seconds(3);
    while (Clock::now() < until)
    {
        mut.lock();
        state = writing;
        assert(state == writing);
        state = reading;
        ++count;
        mut.unlock();
    }
    print("writer = ", count, '\n');
}

void try_reader()
{
    typedef boost::chrono::steady_clock Clock;
    unsigned count = 0;
    Clock::time_point until = Clock::now() + boost::chrono::seconds(3);
    while (Clock::now() < until)
    {
        if (mut.try_lock_shared())
        {
            assert(state == reading);
            ++count;
            mut.unlock_shared();
        }
    }
    print("try_reader = ", count, '\n');
}

void try_writer()
{
    typedef boost::chrono::steady_clock Clock;
    unsigned count = 0;
    Clock::time_point until = Clock::now() + boost::chrono::seconds(3);
    while (Clock::now() < until)
    {
        if (mut.try_lock())
        {
            state = writing;
            assert(state == writing);
            state = reading;
            ++count;
            mut.unlock();
        }
    }
    print("try_writer = ", count, '\n');
}

void try_for_reader()
{
    typedef boost::chrono::steady_clock Clock;
    unsigned count = 0;
    Clock::time_point until = Clock::now() + boost::chrono::seconds(3);
    while (Clock::now() < until)
    {
        if (mut.try_lock_shared_for(boost::chrono::microseconds(5)))
        {
            assert(state == reading);
            ++count;
            mut.unlock_shared();
        }
    }
    print("try_for_reader = ", count, '\n');
}

void try_for_writer()
{
    typedef boost::chrono::steady_clock Clock;
    unsigned count = 0;
    Clock::time_point until = Clock::now() + boost::chrono::seconds(3);
    while (Clock::now() < until)
    {
        if (mut.try_lock_for(boost::chrono::microseconds(5)))
        {
            state = writing;
            assert(state == writing);
            state = reading;
            ++count;
            mut.unlock();
        }
    }
    print("try_for_writer = ", count, '\n');
}

void
test_shared_mutex()
{
  std::cout << __FILE__ << "[" <<__LINE__ << "]" << std::endl;
    {
        boost::thread t1(reader);
        boost::thread t2(writer);
        boost::thread t3(reader);
        t1.join();
        t2.join();
        t3.join();
    }
    std::cout << __FILE__ << "[" <<__LINE__ << "]" << std::endl;
    {
        boost::thread t1(try_reader);
        boost::thread t2(try_writer);
        boost::thread t3(try_reader);
        t1.join();
        t2.join();
        t3.join();
    }
    std::cout << __FILE__ << "[" <<__LINE__ << "]" << std::endl;
    {
        boost::thread t1(try_for_reader);
        boost::thread t2(try_for_writer);
        boost::thread t3(try_for_reader);
        t1.join();
        t2.join();
        t3.join();
    }
    std::cout << __FILE__ << "[" <<__LINE__ << "]" << std::endl;
}

}

namespace U
{

boost::upgrade_mutex mut;

void reader()
{
    typedef boost::chrono::steady_clock Clock;
    unsigned count = 0;
    Clock::time_point until = Clock::now() + boost::chrono::seconds(3);
    while (Clock::now() < until)
    {
        mut.lock_shared();
        assert(state == reading);
        ++count;
        mut.unlock_shared();
    }
    print("reader = ", count, '\n');
}

void writer()
{
    typedef boost::chrono::steady_clock Clock;
    unsigned count = 0;
    Clock::time_point until = Clock::now() + boost::chrono::seconds(3);
    while (Clock::now() < until)
    {
        mut.lock();
        state = writing;
        assert(state == writing);
        state = reading;
        ++count;
        mut.unlock();
    }
    print("writer = ", count, '\n');
}

void try_reader()
{
    typedef boost::chrono::steady_clock Clock;
    unsigned count = 0;
    Clock::time_point until = Clock::now() + boost::chrono::seconds(3);
    while (Clock::now() < until)
    {
        if (mut.try_lock_shared())
        {
            assert(state == reading);
            ++count;
            mut.unlock_shared();
        }
    }
    print("try_reader = ", count, '\n');
}

void try_writer()
{
    typedef boost::chrono::steady_clock Clock;
    unsigned count = 0;
    Clock::time_point until = Clock::now() + boost::chrono::seconds(3);
    while (Clock::now() < until)
    {
        if (mut.try_lock())
        {
            state = writing;
            assert(state == writing);
            state = reading;
            ++count;
            mut.unlock();
        }
    }
    print("try_writer = ", count, '\n');
}

void try_for_reader()
{
    typedef boost::chrono::steady_clock Clock;
    unsigned count = 0;
    Clock::time_point until = Clock::now() + boost::chrono::seconds(3);
    while (Clock::now() < until)
    {
        if (mut.try_lock_shared_for(boost::chrono::microseconds(5)))
        {
            assert(state == reading);
            ++count;
            mut.unlock_shared();
        }
    }
    print("try_for_reader = ", count, '\n');
}

void try_for_writer()
{
    typedef boost::chrono::steady_clock Clock;
    unsigned count = 0;
    Clock::time_point until = Clock::now() + boost::chrono::seconds(3);
    while (Clock::now() < until)
    {
        if (mut.try_lock_for(boost::chrono::microseconds(5)))
        {
            state = writing;
            assert(state == writing);
            state = reading;
            ++count;
            mut.unlock();
        }
    }
    print("try_for_writer = ", count, '\n');
}

void upgradable()
{
    typedef boost::chrono::steady_clock Clock;
    unsigned count = 0;
    Clock::time_point until = Clock::now() + boost::chrono::seconds(3);
    while (Clock::now() < until)
    {
        mut.lock_upgrade();
        assert(state == reading);
        ++count;
        mut.unlock_upgrade();
    }
    print("upgradable = ", count, '\n');
}

void try_upgradable()
{
    typedef boost::chrono::steady_clock Clock;
    unsigned count = 0;
    Clock::time_point until = Clock::now() + boost::chrono::seconds(3);
    while (Clock::now() < until)
    {
        if (mut.try_lock_upgrade())
        {
            assert(state == reading);
            ++count;
            mut.unlock_upgrade();
        }
    }
    print("try_upgradable = ", count, '\n');
}

void try_for_upgradable()
{
    typedef boost::chrono::steady_clock Clock;
    unsigned count = 0;
    Clock::time_point until = Clock::now() + boost::chrono::seconds(3);
    while (Clock::now() < until)
    {
        if (mut.try_lock_upgrade_for(boost::chrono::microseconds(5)))
        {
            assert(state == reading);
            ++count;
            mut.unlock_upgrade();
        }
    }
    print("try_for_upgradable = ", count, '\n');
}

void clockwise()
{
    typedef boost::chrono::steady_clock Clock;
    unsigned count = 0;
    Clock::time_point until = Clock::now() + boost::chrono::seconds(3);
    while (Clock::now() < until)
    {
        mut.lock_shared();
        assert(state == reading);
        if (mut.try_unlock_shared_and_lock())
        {
            state = writing;
        }
        else if (mut.try_unlock_shared_and_lock_upgrade())
        {
            assert(state == reading);
            mut.unlock_upgrade_and_lock();
            state = writing;
        }
        else
        {
            mut.unlock_shared();
            continue;
        }
        assert(state == writing);
        state = reading;
        mut.unlock_and_lock_upgrade();
        assert(state == reading);
        mut.unlock_upgrade_and_lock_shared();
        assert(state == reading);
        mut.unlock_shared();
        ++count;
    }
    print("clockwise = ", count, '\n');
}

void counter_clockwise()
{
    typedef boost::chrono::steady_clock Clock;
    unsigned count = 0;
    Clock::time_point until = Clock::now() + boost::chrono::seconds(3);
    while (Clock::now() < until)
    {
        mut.lock_upgrade();
        assert(state == reading);
        mut.unlock_upgrade_and_lock();
        assert(state == reading);
        state = writing;
        assert(state == writing);
        state = reading;
        mut.unlock_and_lock_shared();
        assert(state == reading);
        mut.unlock_shared();
        ++count;
    }
    print("counter_clockwise = ", count, '\n');
}

void try_clockwise()
{
    typedef boost::chrono::steady_clock Clock;
    unsigned count = 0;
    Clock::time_point until = Clock::now() + boost::chrono::seconds(3);
    while (Clock::now() < until)
    {
        if (mut.try_lock_shared())
        {
            assert(state == reading);
            if (mut.try_unlock_shared_and_lock())
            {
                state = writing;
            }
            else if (mut.try_unlock_shared_and_lock_upgrade())
            {
                assert(state == reading);
                mut.unlock_upgrade_and_lock();
                state = writing;
            }
            else
            {
                mut.unlock_shared();
                continue;
            }
            assert(state == writing);
            state = reading;
            mut.unlock_and_lock_upgrade();
            assert(state == reading);
            mut.unlock_upgrade_and_lock_shared();
            assert(state == reading);
            mut.unlock_shared();
            ++count;
        }
    }
    print("try_clockwise = ", count, '\n');
}

void try_for_clockwise()
{
    typedef boost::chrono::steady_clock Clock;
    unsigned count = 0;
    Clock::time_point until = Clock::now() + boost::chrono::seconds(3);
    while (Clock::now() < until)
    {
        if (mut.try_lock_shared_for(boost::chrono::microseconds(5)))
        {
            assert(state == reading);
            if (mut.try_unlock_shared_and_lock_for(boost::chrono::microseconds(5)))
            {
                state = writing;
            }
            else if (mut.try_unlock_shared_and_lock_upgrade_for(boost::chrono::microseconds(5)))
            {
                assert(state == reading);
                mut.unlock_upgrade_and_lock();
                state = writing;
            }
            else
            {
                mut.unlock_shared();
                continue;
            }
            assert(state == writing);
            state = reading;
            mut.unlock_and_lock_upgrade();
            assert(state == reading);
            mut.unlock_upgrade_and_lock_shared();
            assert(state == reading);
            mut.unlock_shared();
            ++count;
        }
    }
    print("try_for_clockwise = ", count, '\n');
}

void try_counter_clockwise()
{
    typedef boost::chrono::steady_clock Clock;
    unsigned count = 0;
    Clock::time_point until = Clock::now() + boost::chrono::seconds(3);
    while (Clock::now() < until)
    {
        if (mut.try_lock_upgrade())
        {
            assert(state == reading);
            if (mut.try_unlock_upgrade_and_lock())
            {
                assert(state == reading);
                state = writing;
                assert(state == writing);
                state = reading;
                mut.unlock_and_lock_shared();
                assert(state == reading);
                mut.unlock_shared();
                ++count;
            }
            else
            {
                mut.unlock_upgrade();
            }
        }
    }
    print("try_counter_clockwise = ", count, '\n');
}

void try_for_counter_clockwise()
{
    typedef boost::chrono::steady_clock Clock;
    unsigned count = 0;
    Clock::time_point until = Clock::now() + boost::chrono::seconds(3);
    while (Clock::now() < until)
    {
        if (mut.try_lock_upgrade_for(boost::chrono::microseconds(5)))
        {
            assert(state == reading);
            if (mut.try_unlock_upgrade_and_lock_for(boost::chrono::microseconds(5)))
            {
                assert(state == reading);
                state = writing;
                assert(state == writing);
                state = reading;
                mut.unlock_and_lock_shared();
                assert(state == reading);
                mut.unlock_shared();
                ++count;
            }
            else
            {
                mut.unlock_upgrade();
            }
        }
    }
    print("try_for_counter_clockwise = ", count, '\n');
}

void
test_upgrade_mutex()
{
  std::cout << __FILE__ << "[" <<__LINE__ << "]" << std::endl;
    {
        boost::thread t1(reader);
        boost::thread t2(writer);
        boost::thread t3(reader);
        t1.join();
        t2.join();
        t3.join();
    }
    std::cout << __FILE__ << "[" <<__LINE__ << "]" << std::endl;
    {
        boost::thread t1(try_reader);
        boost::thread t2(try_writer);
        boost::thread t3(try_reader);
        t1.join();
        t2.join();
        t3.join();
    }
    std::cout << __FILE__ << "[" <<__LINE__ << "]" << std::endl;
    {
        boost::thread t1(try_for_reader);
        boost::thread t2(try_for_writer);
        boost::thread t3(try_for_reader);
        t1.join();
        t2.join();
        t3.join();
    }
    std::cout << __FILE__ << "[" <<__LINE__ << "]" << std::endl;
    {
        boost::thread t1(reader);
        boost::thread t2(writer);
        boost::thread t3(upgradable);
        t1.join();
        t2.join();
        t3.join();
    }
    std::cout << __FILE__ << "[" <<__LINE__ << "]" << std::endl;
    {
        boost::thread t1(reader);
        boost::thread t2(writer);
        boost::thread t3(try_upgradable);
        t1.join();
        t2.join();
        t3.join();
    }
    std::cout << __FILE__ << "[" <<__LINE__ << "]" << std::endl;
    {
        boost::thread t1(reader);
        boost::thread t2(writer);
        boost::thread t3(try_for_upgradable);
        t1.join();
        t2.join();
        t3.join();
    }
    std::cout << __FILE__ << "[" <<__LINE__ << "]" << std::endl;
    {
        state = reading;
        boost::thread t1(clockwise);
        boost::thread t2(counter_clockwise);
        boost::thread t3(clockwise);
        boost::thread t4(counter_clockwise);
        t1.join();
        t2.join();
        t3.join();
        t4.join();
    }
    std::cout << __FILE__ << "[" <<__LINE__ << "]" << std::endl;
    {
        state = reading;
        boost::thread t1(try_clockwise);
        boost::thread t2(try_counter_clockwise);
        t1.join();
        t2.join();
    }
    std::cout << __FILE__ << "[" <<__LINE__ << "]" << std::endl;
//    {
//        state = reading;
//        boost::thread t1(try_for_clockwise);
//        boost::thread t2(try_for_counter_clockwise);
//        t1.join();
//        t2.join();
//    }
//    std::cout << __FILE__ << "[" <<__LINE__ << "]" << std::endl;
}

}

namespace Assignment
{

class A
{
    typedef boost::upgrade_mutex            mutex_type;
    typedef boost::shared_lock<mutex_type>  SharedLock;
    typedef boost::upgrade_lock<mutex_type> UpgradeLock;
    typedef boost::unique_lock<mutex_type>   Lock;

    mutable mutex_type  mut_;
    std::vector<double> data_;

public:

    A(const A& a)
    {
        SharedLock _(a.mut_);
        data_ = a.data_;
    }

    A& operator=(const A& a)
    {
        if (this != &a)
        {
            Lock       this_lock(mut_, boost::defer_lock);
            SharedLock that_lock(a.mut_, boost::defer_lock);
            boost::lock(this_lock, that_lock);
            data_ = a.data_;
        }
        return *this;
    }

    void swap(A& a)
    {
        Lock this_lock(mut_, boost::defer_lock);
        Lock that_lock(a.mut_, boost::defer_lock);
        boost::lock(this_lock, that_lock);
        data_.swap(a.data_);
    }

    void average(A& a)
    {
        assert(data_.size() == a.data_.size());
        assert(this != &a);

        Lock        this_lock(mut_, boost::defer_lock);
        UpgradeLock share_that_lock(a.mut_, boost::defer_lock);
        boost::lock(this_lock, share_that_lock);

        for (unsigned i = 0; i < data_.size(); ++i)
            data_[i] = (data_[i] + a.data_[i]) / 2;

        SharedLock share_this_lock(boost::move(this_lock));
        Lock that_lock(boost::move(share_that_lock));
        a.data_ = data_;
    }
};

}  // Assignment

void temp()
{
    using namespace boost;
    static upgrade_mutex mut;
    unique_lock<upgrade_mutex> ul(mut);
    shared_lock<upgrade_mutex> sl;
    sl = BOOST_THREAD_MAKE_RV_REF(shared_lock<upgrade_mutex>(boost::move(ul)));
}

int main()
{
  std::cout << __FILE__ << "[" <<__LINE__ << "]" << std::endl;
  typedef boost::chrono::high_resolution_clock Clock;
  typedef boost::chrono::duration<double> sec;
  Clock::time_point t0 = Clock::now();

  std::cout << __FILE__ << "[" <<__LINE__ << "]" << std::endl;
  S::test_shared_mutex();
  std::cout << __FILE__ << "[" <<__LINE__ << "]" << std::endl;
  U::test_upgrade_mutex();
  std::cout << __FILE__ << "[" <<__LINE__ << "]" << std::endl;
  Clock::time_point t1 = Clock::now();
  std::cout << sec(t1 - t0) << '\n';
  return 0;
}

#else
#error "This platform doesn't support Boost.Chrono"
#endif
