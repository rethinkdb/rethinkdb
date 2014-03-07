//Copyright (c) 2006-2009 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "helper1.hpp"
#include <boost/exception/get_error_info.hpp>
#include <boost/exception/info.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/detail/lightweight_test.hpp>

namespace
    {
    typedef boost::error_info<struct tag_test_int,int> test_int;

    void
    throw_wrapper()
        {
        try
            {
            boost::exception_test::throw_length_error();
            }
        catch(
        boost::exception & x )
            {
            x << test_int(42);
            throw;
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
    try
        {
        throw_wrapper();
        BOOST_TEST(false);
        }
    catch(
    std::exception & x )
        {
#ifdef BOOST_NO_RTTI
        try
            {
            throw;
            }
        catch(
        boost::exception & x )
            {
#endif
            BOOST_TEST( boost::get_error_info<test_int>(x) );
            if( int const * p=boost::get_error_info<test_int>(x) )
                BOOST_TEST( 42==*p );
#ifdef BOOST_NO_RTTI
            }
        catch(
        ... )
            {
            BOOST_TEST(false);
            }
#endif
        BOOST_TEST( std::string(x.what())==std::string("exception test length error") );
        }
    catch(
    ... )
        {
        BOOST_TEST(false);
        }
    return boost::report_errors();
    }
