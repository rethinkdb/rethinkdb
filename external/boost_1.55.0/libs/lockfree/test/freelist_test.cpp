//  Copyright (C) 2011 Tim Blechmann
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

// enables error checks via dummy::~dtor
#define BOOST_LOCKFREE_FREELIST_INIT_RUNS_DTOR

#include <boost/lockfree/detail/freelist.hpp>
#include <boost/lockfree/queue.hpp>

#include <boost/foreach.hpp>
#include <boost/thread.hpp>

#define BOOST_TEST_MAIN
#ifdef BOOST_LOCKFREE_INCLUDE_TESTS
#include <boost/test/included/unit_test.hpp>
#else
#include <boost/test/unit_test.hpp>
#endif

#include <boost/foreach.hpp>

#include <set>

#include "test_helpers.hpp"

using boost::lockfree::detail::atomic;

atomic<bool> test_running(false);

struct dummy
{
    dummy(void)
    {
        if (test_running.load(boost::lockfree::detail::memory_order_relaxed))
            assert(allocated == 0);
        allocated = 1;
    }

    ~dummy(void)
    {
        if (test_running.load(boost::lockfree::detail::memory_order_relaxed))
            assert(allocated == 1);
        allocated = 0;
    }

    size_t padding[2]; // for used for the freelist node
    int allocated;
};

template <typename freelist_type,
          bool threadsafe,
          bool bounded>
void run_test(void)
{
    freelist_type fl(std::allocator<int>(), 8);

    std::set<dummy*> nodes;

    dummy d;
    if (bounded)
        test_running.store(true);

    for (int i = 0; i != 4; ++i) {
        dummy * allocated = fl.template construct<threadsafe, bounded>();
        BOOST_REQUIRE(nodes.find(allocated) == nodes.end());
        nodes.insert(allocated);
    }

    BOOST_FOREACH(dummy * d, nodes)
        fl.template destruct<threadsafe>(d);

    nodes.clear();
    for (int i = 0; i != 4; ++i)
        nodes.insert(fl.template construct<threadsafe, bounded>());

    BOOST_FOREACH(dummy * d, nodes)
        fl.template destruct<threadsafe>(d);

    for (int i = 0; i != 4; ++i)
        nodes.insert(fl.template construct<threadsafe, bounded>());

    if (bounded)
        test_running.store(false);
}

template <bool bounded>
void run_tests(void)
{
    run_test<boost::lockfree::detail::freelist_stack<dummy>, true, bounded>();
    run_test<boost::lockfree::detail::freelist_stack<dummy>, false, bounded>();
    run_test<boost::lockfree::detail::fixed_size_freelist<dummy>, true, bounded>();
}

BOOST_AUTO_TEST_CASE( freelist_tests )
{
    run_tests<false>();
    run_tests<true>();
}

template <typename freelist_type, bool threadsafe>
void oom_test(void)
{
    const bool bounded = true;
    freelist_type fl(std::allocator<int>(), 8);

    for (int i = 0; i != 8; ++i)
        fl.template construct<threadsafe, bounded>();

    dummy * allocated = fl.template construct<threadsafe, bounded>();
    BOOST_REQUIRE(allocated == NULL);
}

BOOST_AUTO_TEST_CASE( oom_tests )
{
    oom_test<boost::lockfree::detail::freelist_stack<dummy>, true >();
    oom_test<boost::lockfree::detail::freelist_stack<dummy>, false >();
    oom_test<boost::lockfree::detail::fixed_size_freelist<dummy>, true >();
    oom_test<boost::lockfree::detail::fixed_size_freelist<dummy>, false >();
}


template <typename freelist_type, bool bounded>
struct freelist_tester
{
    static const int size = 128;
    static const int thread_count = 4;
#ifndef BOOST_LOCKFREE_STRESS_TEST
    static const int operations_per_thread = 1000;
#else
    static const int operations_per_thread = 100000;
#endif

    freelist_type fl;
    boost::lockfree::queue<dummy*> allocated_nodes;

    atomic<bool> running;
    static_hashed_set<dummy*, 1<<16 > working_set;


    freelist_tester(void):
        fl(std::allocator<int>(), size), allocated_nodes(256)
    {}

    void run()
    {
        running = true;

        if (bounded)
            test_running.store(true);
        boost::thread_group alloc_threads;
        boost::thread_group dealloc_threads;

        for (int i = 0; i != thread_count; ++i)
            dealloc_threads.create_thread(boost::bind(&freelist_tester::deallocate, this));

        for (int i = 0; i != thread_count; ++i)
            alloc_threads.create_thread(boost::bind(&freelist_tester::allocate, this));
        alloc_threads.join_all();
        test_running.store(false);
        running = false;
        dealloc_threads.join_all();
    }

    void allocate(void)
    {
        for (long i = 0; i != operations_per_thread; ++i)  {
            for (;;) {
                dummy * node = fl.template construct<true, bounded>();
                if (node) {
                    bool success = working_set.insert(node);
                    assert(success);
                    allocated_nodes.push(node);
                    break;
                }
            }
        }
    }

    void deallocate(void)
    {
        for (;;) {
            dummy * node;
            if (allocated_nodes.pop(node)) {
                bool success = working_set.erase(node);
                assert(success);
                fl.template destruct<true>(node);
            }

            if (running.load() == false)
                break;
        }

        dummy * node;
        while (allocated_nodes.pop(node)) {
            bool success = working_set.erase(node);
            assert(success);
            fl.template destruct<true>(node);
        }
    }
};

template <typename Tester>
void run_tester()
{
    boost::scoped_ptr<Tester> tester (new Tester);
    tester->run();
}


BOOST_AUTO_TEST_CASE( unbounded_freelist_test )
{
    typedef freelist_tester<boost::lockfree::detail::freelist_stack<dummy>, false > test_type;
    run_tester<test_type>();
}


BOOST_AUTO_TEST_CASE( bounded_freelist_test )
{
    typedef freelist_tester<boost::lockfree::detail::freelist_stack<dummy>, true > test_type;
    run_tester<test_type>();
}

BOOST_AUTO_TEST_CASE( fixed_size_freelist_test )
{
    typedef freelist_tester<boost::lockfree::detail::fixed_size_freelist<dummy>, true > test_type;
    run_tester<test_type>();
}
