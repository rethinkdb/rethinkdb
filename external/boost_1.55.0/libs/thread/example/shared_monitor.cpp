// Copyright (C) 2012 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/lock_algorithms.hpp>
#include <boost/thread/thread_only.hpp>
#if defined BOOST_THREAD_DONT_USE_CHRONO
#include <boost/chrono/chrono_io.hpp>
#endif
#include <cassert>
#include <vector>

#define EXCLUSIVE 1
#define SHARED 2

#define MODE SHARED

class A
{
#if MODE == EXCLUSIVE
    typedef boost::mutex mutex_type;
#elif MODE == SHARED
    typedef boost::shared_mutex mutex_type;
#else
#error MODE not set
#endif
    typedef std::vector<double> C;
    mutable mutex_type mut_;
    C data_;
public:
    A() : data_(10000000) {}
    A(const A& a);
    A& operator=(const A& a);

    void compute(const A& x, const A& y);
};

A::A(const A& a)
{
#if MODE == EXCLUSIVE
    boost::unique_lock<mutex_type> lk(a.mut_);
#elif MODE == SHARED
    boost::shared_lock<mutex_type> lk(a.mut_);
#else
#error MODE not set
#endif
    data_ = a.data_;
}

A&
A::operator=(const A& a)
{
    if (this != &a)
    {
        boost::unique_lock<mutex_type> lk1(mut_, boost::defer_lock);
#if MODE == EXCLUSIVE
        boost::unique_lock<mutex_type> lk2(a.mut_, boost::defer_lock);
#elif MODE == SHARED
        boost::shared_lock<mutex_type> lk2(a.mut_, boost::defer_lock);
#else
#error MODE not set
#endif
        boost::lock(lk1, lk2);
        data_ = a.data_;
    }
    return *this;
}

void
A::compute(const A& x, const A& y)
{
    boost::unique_lock<mutex_type> lk1(mut_, boost::defer_lock);
#if MODE == EXCLUSIVE
    boost::unique_lock<mutex_type> lk2(x.mut_, boost::defer_lock);
    boost::unique_lock<mutex_type> lk3(y.mut_, boost::defer_lock);
#elif MODE == SHARED
    boost::shared_lock<mutex_type> lk2(x.mut_, boost::defer_lock);
    boost::shared_lock<mutex_type> lk3(y.mut_, boost::defer_lock);
#else
#error MODE not set
#endif
    boost::lock(lk1, lk2, lk3);
    assert(data_.size() == x.data_.size());
    assert(data_.size() == y.data_.size());
    for (unsigned i = 0; i < data_.size(); ++i)
        data_[i] = (x.data_[i] + y.data_[i]) / 2;
}

A a1;
A a2;

void test_s()
{
    A la3 = a1;
    for (int i = 0; i < 150; ++i)
    {
        la3.compute(a1, a2);
    }
}

void test_w()
{
    A la3 = a1;
    for (int i = 0; i < 10; ++i)
    {
        la3.compute(a1, a2);
        a1 = la3;
        a2 = la3;
#if defined BOOST_THREAD_DONT_USE_CHRONO
        boost::this_thread::sleep_for(boost::chrono::seconds(1));
#endif
    }
}

int main()
{
#if defined BOOST_THREAD_DONT_USE_CHRONO
    typedef boost::chrono::high_resolution_clock Clock;
    typedef boost::chrono::duration<double> sec;
    Clock::time_point t0 = Clock::now();
#endif
    std::vector<boost::thread*> v;
    boost::thread thw(test_w);
    v.push_back(&thw);
    boost::thread thr0(test_w);
    v.push_back(&thr0);
    boost::thread thr1(test_w);
    v.push_back(&thr1);
    boost::thread thr2(test_w);
    v.push_back(&thr2);
    boost::thread thr3(test_w);
    v.push_back(&thr3);
    for (std::size_t i = 0; i < v.size(); ++i)
        v[i]->join();
#if defined BOOST_THREAD_DONT_USE_CHRONO
    Clock::time_point t1 = Clock::now();
    std::cout << sec(t1-t0) << '\n';
#endif
    return 0;
}
