//Copyright (c) 2006-2009 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "helper2.hpp"
#include <boost/exception/get_error_info.hpp>
#include <boost/exception/info.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/detail/lightweight_test.hpp>

typedef boost::error_info<struct tag_test_int,int> test_data;

struct
exception1:
    std::exception
    {
    };

struct
exception2:
    std::exception,
    boost::exception
    {
    };

void
boost_throw_exception_test()
    {
    try
        {
        BOOST_THROW_EXCEPTION(exception1());
        BOOST_ERROR("BOOST_THROW_EXCEPTION failed to throw.");
        }
    catch(
    boost::exception & x )
        {
        char const * const * function=boost::get_error_info<boost::throw_function>(x);
        char const * const * file=boost::get_error_info<boost::throw_file>(x);
        int const * line=boost::get_error_info<boost::throw_line>(x);
        BOOST_TEST( file && *file );
        BOOST_TEST( function && *function );
        BOOST_TEST( line && *line==32 );
        }
    catch(
    ... )
        {
        BOOST_TEST(false);
        }
    try
        {
        BOOST_THROW_EXCEPTION(exception2() << test_data(42));
        BOOST_ERROR("BOOST_THROW_EXCEPTION failed to throw.");
        }
    catch(
    boost::exception & x )
        {
        char const * const * function=boost::get_error_info<boost::throw_function>(x);
        char const * const * file=boost::get_error_info<boost::throw_file>(x);
        int const * line=boost::get_error_info<boost::throw_line>(x);
        int const * data=boost::get_error_info<test_data>(x);
        BOOST_TEST( file && *file );
        BOOST_TEST( function && *function );
        BOOST_TEST( line && *line==52 );
        BOOST_TEST( data && *data==42 );
        }
    catch(
    ... )
        {
        BOOST_TEST(false);
        }
    }

void
throw_fwd( void (*thrower)(int) )
    {
    try
        {
        thrower(42);
        }
    catch(
    boost::exception & x )
        {
        x << test_data(42);
        throw;
        }
    }

template <class T>
void
tester()
    {
    try
        {
        throw_fwd( &boost::exception_test::throw_test_exception<T> );
        BOOST_ASSERT(false);
        }
    catch(
    ... )
        {
        boost::exception_ptr p = boost::current_exception();
        try
            {
            rethrow_exception(p);
            BOOST_TEST(false);
            }
        catch(
        T & y )
            {
#ifdef BOOST_NO_RTTI
            try
                {
                throw;
                }
            catch(
            boost::exception & y )
                {
#endif
                BOOST_TEST(boost::get_error_info<test_data>(y));
                if( int const * d=boost::get_error_info<test_data>(y) )
                    BOOST_TEST(*d==42);
#ifdef BOOST_NO_RTTI
                }
            catch(
            ... )
                {
                BOOST_TEST(false);
                }
#endif
            BOOST_TEST(y.x_==42);
            }
        catch(
        ... )
            {
            BOOST_TEST(false);
            }
        }
    }

int
main()
    {
    boost_throw_exception_test();
    tester<boost::exception_test::derives_boost_exception>();
    tester<boost::exception_test::derives_boost_exception_virtually>();
    tester<boost::exception_test::derives_std_exception>();
    return boost::report_errors();
    }
