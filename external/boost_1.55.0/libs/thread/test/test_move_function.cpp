// Copyright (C) 2007-8 Anthony Williams
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_THREAD_VERSION 2

#include <boost/thread/thread_only.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp>

void do_nothing()
{}

void test_thread_move_from_lvalue_on_construction()
{
    boost::thread src(do_nothing);
    boost::thread::id src_id=src.get_id();
    boost::thread dest(boost::move(src));
    boost::thread::id dest_id=dest.get_id();
    BOOST_CHECK(src_id==dest_id);
    BOOST_CHECK(src.get_id()==boost::thread::id());
    dest.join();
}

void test_thread_move_from_lvalue_on_assignment()
{
    boost::thread src(do_nothing);
    boost::thread::id src_id=src.get_id();
    boost::thread dest;
    dest=boost::move(src);
    boost::thread::id dest_id=dest.get_id();
    BOOST_CHECK(src_id==dest_id);
    BOOST_CHECK(src.get_id()==boost::thread::id());
    dest.join();
}

boost::thread start_thread()
{
    return boost::thread(do_nothing);
}

void test_thread_move_from_rvalue_on_construction()
{
    boost::thread x(start_thread());
    BOOST_CHECK(x.get_id()!=boost::thread::id());
    x.join();
}

void test_thread_move_from_rvalue_using_explicit_move()
{
    //boost::thread x(boost::move(start_thread()));
    boost::thread x=start_thread();
    BOOST_CHECK(x.get_id()!=boost::thread::id());
    x.join();
}

void test_unique_lock_move_from_lvalue_on_construction()
{
    boost::mutex m;
    boost::unique_lock<boost::mutex> l(m);
    BOOST_CHECK(l.owns_lock());
    BOOST_CHECK(l.mutex()==&m);

    boost::unique_lock<boost::mutex> l2(boost::move(l));
    BOOST_CHECK(!l.owns_lock());
    BOOST_CHECK(!l.mutex());
    BOOST_CHECK(l2.owns_lock());
    BOOST_CHECK(l2.mutex()==&m);
}

boost::unique_lock<boost::mutex> get_lock(boost::mutex& m)
{
    return boost::unique_lock<boost::mutex>(m);
}


void test_unique_lock_move_from_rvalue_on_construction()
{
    boost::mutex m;
    boost::unique_lock<boost::mutex> l(get_lock(m));
    BOOST_CHECK(l.owns_lock());
    BOOST_CHECK(l.mutex()==&m);
}

namespace user_test_ns
{
    template<typename T>
    T move(T& t)
    {
        return t.move();
    }

    bool move_called=false;

    struct nc:
        public boost::shared_ptr<int>
    {
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
        nc() {}
        nc(nc&&)
        {
            move_called=true;
        }
#endif
        nc move()
        {
            move_called=true;
            return nc();
        }
    };
}

namespace boost
{
    BOOST_THREAD_DCL_MOVABLE(user_test_ns::nc)
}

void test_move_for_user_defined_type_unaffected()
{
    user_test_ns::nc src;
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    user_test_ns::nc dest=boost::move(src);
#else
    user_test_ns::nc dest=move(src);
#endif
    BOOST_CHECK(user_test_ns::move_called);
}

boost::unit_test::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test::test_suite* test =
        BOOST_TEST_SUITE("Boost.Threads: thread move test suite");

    test->add(BOOST_TEST_CASE(test_thread_move_from_lvalue_on_construction));
    test->add(BOOST_TEST_CASE(test_thread_move_from_rvalue_on_construction));
    test->add(BOOST_TEST_CASE(test_thread_move_from_rvalue_using_explicit_move));
    test->add(BOOST_TEST_CASE(test_thread_move_from_lvalue_on_assignment));
    test->add(BOOST_TEST_CASE(test_unique_lock_move_from_lvalue_on_construction));
    test->add(BOOST_TEST_CASE(test_unique_lock_move_from_rvalue_on_construction));
    test->add(BOOST_TEST_CASE(test_move_for_user_defined_type_unaffected));
    return test;
}


