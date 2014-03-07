//Copyright (c) 2006-2009 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/exception_ptr.hpp>
#include <boost/exception/get_error_info.hpp>
#include <boost/thread.hpp>
#include <boost/detail/atomic_count.hpp>
#include <boost/detail/lightweight_test.hpp>

typedef boost::error_info<struct tag_answer,int> answer;

boost::detail::atomic_count exc_count(0);

struct
err:
    virtual boost::exception,
    virtual std::exception
    {
    err()
        {
        ++exc_count;
        }

    err( err const & )
        {
        ++exc_count;
        }

    virtual
    ~err() throw()
        {
        --exc_count;
        }

    private:

    err & operator=( err const & );
    };

class
future
    {
    public:

    future ():
        ready_ (false)
        {
        }

    void
    set_exception( boost::exception_ptr const & e )
        {
        boost::unique_lock<boost::mutex> lck (mux_);
        exc_ = e;
        ready_ = true;
        cond_.notify_all();
        }

    void
    get_exception() const
        {
        boost::unique_lock<boost::mutex> lck (mux_);
        while (! ready_)
            cond_.wait (lck);
        rethrow_exception (exc_);
        }

    private:

    bool ready_;
    boost::exception_ptr exc_;
    mutable boost::mutex mux_;
    mutable boost::condition_variable cond_;
    };

void
producer( future & f )
    {
    f.set_exception (boost::copy_exception (err () << answer(42)));
    }

void
consumer()
    {
    future f;
    boost::thread thr (boost::bind (&producer, boost::ref (f)));
    try
        {
        f.get_exception ();
        }
    catch(
    err & e )
        {
        int const * ans=boost::get_error_info<answer>(e);
        BOOST_TEST(ans && *ans==42);
        }
    thr.join();
    }

void
consume()
    {
    for( int i=0; i!=100; ++i )
        consumer();
    }

void
thread_test()
    {
    boost::thread_group grp;
    for( int i=0; i!=50; ++i )
        grp.create_thread(&consume);
    grp.join_all ();
    }

void
simple_test()
    {
    boost::exception_ptr p = boost::copy_exception(err());
    try
        {
        rethrow_exception(p);
        BOOST_TEST(false);
        }
    catch(
    err & )
        {
        }
    catch(
    ... )
        {
        BOOST_TEST(false);
        }
    }

int
main()
    {
    BOOST_TEST(++exc_count==1);
    simple_test();
    thread_test();
    BOOST_TEST(!--exc_count);
    return boost::report_errors();
    }
