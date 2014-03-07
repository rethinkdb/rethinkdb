//Copyright (c) 2006-2009 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//This example shows how to transport cloning-enabled boost::exceptions between threads.

#include <boost/exception_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

void do_work(); //throws cloning-enabled boost::exceptions

void
worker_thread( boost::exception_ptr & error )
    {
    try
        {
        do_work();
        error = boost::exception_ptr();
        }
    catch(
    ... )
        {
        error = boost::current_exception();
        }
    }

// ...continued

void
work()
    {
    boost::exception_ptr error;
    boost::thread t( boost::bind(worker_thread,boost::ref(error)) );
    t.join();
    if( error )
        boost::rethrow_exception(error);
    }
