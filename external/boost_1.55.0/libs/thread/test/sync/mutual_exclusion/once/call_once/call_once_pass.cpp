//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// Copyright (C) 2013 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// <boost/thread/once.hpp>

// struct once_flag;

// template<class Callable, class ...Args>
//   void call_once(once_flag& flag, Callable&& func, Args&&... args);

//#define BOOST_THREAD_VERSION 4
#define BOOST_THREAD_USES_MOVE
#define BOOST_THREAD_PROVIDES_ONCE_CXX11

#include <boost/thread/once.hpp>
#include <boost/thread/thread.hpp>
#include <boost/detail/lightweight_test.hpp>

#ifdef BOOST_THREAD_PROVIDES_ONCE_CXX11
#define BOOST_INIT_ONCE_INIT
#else
#define BOOST_INIT_ONCE_INIT =BOOST_ONCE_INIT
#endif

typedef boost::chrono::milliseconds ms;

boost::once_flag flg0 BOOST_INIT_ONCE_INIT;

int init0_called = 0;

void init0()
{
    boost::this_thread::sleep_for(ms(250));
    ++init0_called;
}

void f0()
{
    boost::call_once(flg0, init0);
}

boost::once_flag flg3 BOOST_INIT_ONCE_INIT;

int init3_called = 0;
int init3_completed = 0;

void init3()
{
    ++init3_called;
    boost::this_thread::sleep_for(ms(250));
    if (init3_called == 1)
        throw 1;
    ++init3_completed;
}

void f3()
{
    try
    {
        boost::call_once(flg3, init3);
    }
    catch (...)
    {
    }
}

struct init1
{
    static int called;
    typedef void result_type;

    void operator()(int i) {called += i;}
    void operator()(int i) const {called += i;}
};

int init1::called = 0;

boost::once_flag flg1 BOOST_INIT_ONCE_INIT;

void f1()
{
    boost::call_once(flg1, init1(), 1);
}

boost::once_flag flg1_member BOOST_INIT_ONCE_INIT;

struct init1_member
{
    static int called;
    typedef void result_type;
    void call(int i) {
      called += i;
    }
};
int init1_member::called = 0;

//#if defined BOOST_THREAD_PLATFORM_PTHREAD
void f1_member()
{
    init1_member o;
//#if defined BOOST_THREAD_PROVIDES_ONCE_CXX11
    boost::call_once(flg1_member, &init1_member::call, o, 1);
//#else
//    boost::call_once(flg1_member, boost::bind(&init1_member::call, boost::ref(o), 1));
//#endif
}
//#endif
struct init2
{
    static int called;
    typedef void result_type;

    void operator()(int i, int j) {called += i + j;}
    void operator()(int i, int j) const {called += i + j;}
};

int init2::called = 0;

boost::once_flag flg2 BOOST_INIT_ONCE_INIT;

void f2()
{
    boost::call_once(flg2, init2(), 2, 3);
    boost::call_once(flg2, init2(), 4, 5);
}

boost::once_flag flg41 BOOST_INIT_ONCE_INIT;
boost::once_flag flg42 BOOST_INIT_ONCE_INIT;

int init41_called = 0;
int init42_called = 0;

void init42();

void init41()
{
    boost::this_thread::sleep_for(ms(250));
    ++init41_called;
}

void init42()
{
    boost::this_thread::sleep_for(ms(250));
    ++init42_called;
}

void f41()
{
    boost::call_once(flg41, init41);
    boost::call_once(flg42, init42);
}

void f42()
{
    boost::call_once(flg42, init42);
    boost::call_once(flg41, init41);
}

class MoveOnly
{
public:
  typedef void result_type;

  BOOST_THREAD_MOVABLE_ONLY(MoveOnly)
  MoveOnly()
  {
  }
  MoveOnly(BOOST_THREAD_RV_REF(MoveOnly))
  {}

  void operator()(BOOST_THREAD_RV_REF(MoveOnly))
  {
  }
  void operator()(int)
  {
  }
  void operator()()
  {
  }
};


struct id_string
{
    static boost::once_flag flag;
    static void do_init(id_string & )
    {}
    void operator()()
    {
      boost::call_once(flag, &id_string::do_init, boost::ref(*this));
    }
//    void operator()(int,int)
//    {
//      // This should fail but works with gcc-4.6.3
//      //std::bind(&id_string::do_init, *this)();
//      std::bind(&id_string::do_init, std::ref(*this))();
//    }
//    void operator()(int) const
//    {
//      //std::bind(&id_string::do_init, *this)();
//    }
};


boost::once_flag id_string::flag BOOST_INIT_ONCE_INIT;

int main()
{

  //
  {
    id_string id;
    id();
    //id(1,1);
  }
    // check basic functionality
    {
        boost::thread t0(f0);
        boost::thread t1(f0);
        t0.join();
        t1.join();
        BOOST_TEST(init0_called == 1);
    }
    // check basic exception safety
    {
        boost::thread t0(f3);
        boost::thread t1(f3);
        t0.join();
        t1.join();
        BOOST_TEST(init3_called == 2);
        BOOST_TEST(init3_completed == 1);
    }
    // check deadlock avoidance
    {
        boost::thread t0(f41);
        boost::thread t1(f42);
        t0.join();
        t1.join();
        BOOST_TEST(init41_called == 1);
        BOOST_TEST(init42_called == 1);
    }
    // check functors with 1 arg
    {
        boost::thread t0(f1);
        boost::thread t1(f1);
        t0.join();
        t1.join();
        BOOST_TEST(init1::called == 1);
    }
    // check functors with 2 args
    {
        boost::thread t0(f2);
        boost::thread t1(f2);
        t0.join();
        t1.join();
        BOOST_TEST(init2::called == 5);
    }

    // check member function with 1 arg
    {
        boost::thread t0(f1_member);
        boost::thread t1(f1_member);
        t0.join();
        t1.join();
        BOOST_TEST(init1_member::called == 1);
    }
#if defined BOOST_THREAD_PLATFORM_PTHREAD || (__GNUC__*10000 + __GNUC_MINOR__*100 + __GNUC_PATCHLEVEL__ > 40600)
    {
        boost::once_flag f BOOST_INIT_ONCE_INIT;
        boost::call_once(f, MoveOnly());
    }
#endif
#if defined BOOST_THREAD_PROVIDES_INVOKE
    {
        boost::once_flag f BOOST_INIT_ONCE_INIT;
        boost::call_once(f, MoveOnly(), 1);
    }
    {
        boost::once_flag f BOOST_INIT_ONCE_INIT;
        boost::call_once(f, MoveOnly(), MoveOnly());
    }
#endif  // BOOST_THREAD_PLATFORM_PTHREAD
    return boost::report_errors();
}


