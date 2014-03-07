//Copyright (c) 2006-2009 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/exception_ptr.hpp>
#include <boost/exception/get_error_info.hpp>
#include <boost/exception/info.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/detail/workaround.hpp>

#if BOOST_WORKAROUND(__CODEGEARC__, BOOST_TESTED_AT(0x610))
struct tag_test {};
#endif

typedef boost::error_info<struct tag_test,int> test;

struct
test_boost_exception:
    boost::exception
    {
    };

void
throw_boost_exception()
    {
    throw test_boost_exception() << test(42);
    }

void
throw_unknown_exception()
    {
    struct
    test_exception:
        std::exception
        {
        };
    throw test_exception();
    }

int
main()
    {
    try
        {
        throw_boost_exception();
        }
    catch(
    ... )
        {
        boost::exception_ptr ep=boost::current_exception();
        try
            {
            rethrow_exception(ep);
            }
        catch(
        boost::unknown_exception & x )
            {
            if( int const * d=boost::get_error_info<test>(x) )
                BOOST_TEST( 42==*d );
            else
                BOOST_TEST(false);
            }
        catch(
        boost::exception & x )
            {
            //Yay! Non-intrusive cloning supported!
            if( int const * d=boost::get_error_info<test>(x) )
                BOOST_TEST( 42==*d );
            else
                BOOST_TEST(false);
            }
        catch(
        ... )
            {
            BOOST_TEST(false);
            }
        try
            {
            rethrow_exception(ep);
            }
        catch(
        boost::exception & x )
            {
            if( int const * d=boost::get_error_info<test>(x) )
                BOOST_TEST( 42==*d );
            else
                BOOST_TEST(false);
            }
        catch(
        ... )
            {
            BOOST_TEST(false);
            }
        }
    try
        {
        throw_unknown_exception();
        }
    catch(
    ... )
        {
        boost::exception_ptr ep=boost::current_exception();
        try
            {
            rethrow_exception(ep);
            }
        catch(
        boost::unknown_exception & )
            {
            }
        catch(
        std::exception & )
            {
            //Yay! Non-intrusive cloning supported!
            }
        catch(
        ... )
            {
            BOOST_TEST(false);
            }
        try
            {
            rethrow_exception(ep);
            }
        catch(
        boost::exception & )
            {
            }
        catch(
        std::exception & )
            {
            //Yay! Non-intrusive cloning supported!
            }
        catch(
        ... )
            {
            BOOST_TEST(false);
            }
        }
    return boost::report_errors();
    }
