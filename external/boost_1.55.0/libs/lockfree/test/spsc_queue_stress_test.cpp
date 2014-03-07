//  Copyright (C) 2011-2013 Tim Blechmann
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/lockfree/spsc_queue.hpp>
#include <boost/thread.hpp>

#define BOOST_TEST_MAIN
#ifdef BOOST_LOCKFREE_INCLUDE_TESTS
#include <boost/test/included/unit_test.hpp>
#else
#include <boost/test/unit_test.hpp>
#endif

#include <iostream>
#include <memory>

#include "test_helpers.hpp"
#include "test_common.hpp"

using namespace boost;
using namespace boost::lockfree;
using namespace std;

#ifndef BOOST_LOCKFREE_STRESS_TEST
static const boost::uint32_t nodes_per_thread = 100000;
#else
static const boost::uint32_t nodes_per_thread = 100000000;
#endif

struct spsc_queue_tester
{
    spsc_queue<int, capacity<128> > sf;

    boost::lockfree::detail::atomic<long> spsc_queue_cnt, received_nodes;

    static_hashed_set<int, 1<<16 > working_set;

    spsc_queue_tester(void):
        spsc_queue_cnt(0), received_nodes(0)
    {}

    void add(void)
    {
        for (boost::uint32_t i = 0; i != nodes_per_thread; ++i) {
            int id = generate_id<int>();
            working_set.insert(id);

            while (sf.push(id) == false)
            {}

            ++spsc_queue_cnt;
        }
        running = false;
    }

    bool get_element(void)
    {
        int data;
        bool success = sf.pop(data);

        if (success) {
            ++received_nodes;
            --spsc_queue_cnt;
            bool erased = working_set.erase(data);
            assert(erased);
            return true;
        } else
            return false;
    }

    boost::lockfree::detail::atomic<bool> running;

    void get(void)
    {
        for(;;) {
            bool success = get_element();
            if (!running && !success)
                break;
        }

        while ( get_element() );
    }

    void run(void)
    {
        running = true;

        BOOST_REQUIRE(sf.empty());

        boost::thread reader(boost::bind(&spsc_queue_tester::get, this));
        boost::thread writer(boost::bind(&spsc_queue_tester::add, this));
        cout << "reader and writer threads created" << endl;

        writer.join();
        cout << "writer threads joined. waiting for readers to finish" << endl;

        reader.join();

        BOOST_REQUIRE_EQUAL(received_nodes, nodes_per_thread);
        BOOST_REQUIRE_EQUAL(spsc_queue_cnt, 0);
        BOOST_REQUIRE(sf.empty());
        BOOST_REQUIRE(working_set.count_nodes() == 0);
    }
};

BOOST_AUTO_TEST_CASE( spsc_queue_test_caching )
{
    boost::shared_ptr<spsc_queue_tester> test1(new spsc_queue_tester);
    test1->run();
}

struct spsc_queue_tester_buffering
{
    spsc_queue<int, capacity<128> > sf;

    boost::lockfree::detail::atomic<long> spsc_queue_cnt;

    static_hashed_set<int, 1<<16 > working_set;
    boost::lockfree::detail::atomic<size_t> received_nodes;

    spsc_queue_tester_buffering(void):
        spsc_queue_cnt(0), received_nodes(0)
    {}

    static const size_t buf_size = 5;

    void add(void)
    {
        boost::array<int, buf_size> input_buffer;
        for (boost::uint32_t i = 0; i != nodes_per_thread; i+=buf_size) {
            for (size_t i = 0; i != buf_size; ++i) {
                int id = generate_id<int>();
                working_set.insert(id);
                input_buffer[i] = id;
            }

            size_t pushed = 0;

            do {
                pushed += sf.push(input_buffer.c_array() + pushed,
                                  input_buffer.size()    - pushed);
            } while (pushed != buf_size);

            spsc_queue_cnt+=buf_size;
        }
        running = false;
    }

    bool get_elements(void)
    {
        boost::array<int, buf_size> output_buffer;

        size_t popd = sf.pop(output_buffer.c_array(), output_buffer.size());

        if (popd) {
            received_nodes += popd;
            spsc_queue_cnt -= popd;

            for (size_t i = 0; i != popd; ++i) {
                bool erased = working_set.erase(output_buffer[i]);
                assert(erased);
            }

            return true;
        } else
            return false;
    }

    boost::lockfree::detail::atomic<bool> running;

    void get(void)
    {
        for(;;) {
            bool success = get_elements();
            if (!running && !success)
                break;
        }

        while ( get_elements() );
    }

    void run(void)
    {
        running = true;

        boost::thread reader(boost::bind(&spsc_queue_tester_buffering::get, this));
        boost::thread writer(boost::bind(&spsc_queue_tester_buffering::add, this));
        cout << "reader and writer threads created" << endl;

        writer.join();
        cout << "writer threads joined. waiting for readers to finish" << endl;

        reader.join();

        BOOST_REQUIRE_EQUAL(received_nodes, nodes_per_thread);
        BOOST_REQUIRE_EQUAL(spsc_queue_cnt, 0);
        BOOST_REQUIRE(sf.empty());
        BOOST_REQUIRE(working_set.count_nodes() == 0);
    }
};


BOOST_AUTO_TEST_CASE( spsc_queue_test_buffering )
{
    boost::shared_ptr<spsc_queue_tester_buffering> test1(new spsc_queue_tester_buffering);
    test1->run();
}

