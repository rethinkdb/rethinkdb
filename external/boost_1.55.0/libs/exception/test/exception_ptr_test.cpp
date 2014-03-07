//Copyright (c) 2006-2009 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/exception_ptr.hpp>
#include <boost/exception/info.hpp>
#include <boost/exception/get_error_info.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/detail/atomic_count.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <iostream>

class thread_handle;
boost::shared_ptr<thread_handle> create_thread( boost::function<void()> const & f );
void join( thread_handle & t );

class
thread_handle
    {
    thread_handle( thread_handle const & );
    thread_handle & operator=( thread_handle const & );

    boost::exception_ptr err_;
    boost::thread t_;

    static
    void
    thread_wrapper( boost::function<void()> const & f, boost::exception_ptr & ep )
        {
        BOOST_ASSERT(!ep);
        try
            {
            f();
            }
        catch(...)
            {
            ep = boost::current_exception();
            }
        }

    explicit
    thread_handle( boost::function<void()> const & f ):
        t_(boost::bind(thread_wrapper,f,boost::ref(err_)))
        {
        }

    friend boost::shared_ptr<thread_handle> create_thread( boost::function<void()> const & f );
    friend void join( thread_handle & t );
    };

boost::shared_ptr<thread_handle>
create_thread( boost::function<void()> const & f )
    {
    boost::shared_ptr<thread_handle> t( new thread_handle(f) );
    return t;
    }

void
join( thread_handle & t )
    {
    t.t_.join();
    assert(t.err_);
    rethrow_exception(t.err_);
    }

boost::detail::atomic_count exc_count(0);

struct
exc:
    virtual boost::exception,
    virtual std::exception
    {
    exc()
        {
        ++exc_count;
        }

    exc( exc const & e ):
        boost::exception(e),
        std::exception(e)
        {
        ++exc_count;
        }

    virtual
    ~exc() throw()
        {
        --exc_count;
        }

    private:

    exc & operator=( exc const & );
    };

typedef boost::error_info<struct answer_,int> answer;

void
thread_func()
    {
    BOOST_THROW_EXCEPTION(exc() << answer(42));
    }

void
check( boost::shared_ptr<thread_handle> const & t )
    {
    try
        {
        join(*t);
        BOOST_TEST(false);
        }
    catch(
    exc & e )
        {
        int const * a = boost::get_error_info<answer>(e);
        BOOST_TEST(a && *a==42);
        }
    }

int
main()
    {
    BOOST_TEST(++exc_count==1);
    try
        {
        std::vector< boost::shared_ptr<thread_handle> > threads;
        std::generate_n(std::inserter(threads,threads.end()),1,boost::bind(create_thread,thread_func));
        std::for_each(threads.begin(),threads.end(),check);
        return boost::report_errors();
        }
    catch(
    ... )
        {
        std::cerr <<
            "Caught unexpected exception.\n"
            "Output from current_exception_diagnostic_information:\n" <<
            boost::current_exception_diagnostic_information() << std::endl;
        return 42;
        }
    BOOST_TEST(!--exc_count);
    }
