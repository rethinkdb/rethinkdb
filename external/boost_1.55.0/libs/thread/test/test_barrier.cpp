// Copyright (C) 2001-2003
// William E. Kempf
// (C) Copyright 2013 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_THREAD_PROVIDES_INTERRUPTIONS

#include <boost/thread/detail/config.hpp>

#include <boost/thread/thread.hpp>
#include <boost/thread/barrier.hpp>

#include <boost/detail/lightweight_test.hpp>
#include <vector>

namespace {

// Shared variables for generation barrier test
const int N_THREADS=3;
boost::barrier gen_barrier(N_THREADS);
boost::mutex mutex;
long global_parameter;

void barrier_thread()
{
    for (int i = 0; i < 5; ++i)
    {
        if (gen_barrier.wait())
        {
            boost::unique_lock<boost::mutex> lock(mutex);
            global_parameter++;
        }
    }
}

} // namespace

void test_barrier()
{
    boost::thread_group g;
    global_parameter = 0;

    try
    {
        for (int i = 0; i < N_THREADS; ++i)
            g.create_thread(&barrier_thread);
        g.join_all();
    }
    catch(...)
    {
        g.interrupt_all();
        g.join_all();
        throw;
    }

    BOOST_TEST(global_parameter==5);

}

int main()
{

    test_barrier();
    return boost::report_errors();
}

