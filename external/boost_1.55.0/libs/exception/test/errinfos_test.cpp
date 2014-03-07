//Copyright (c) 2006-2009 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/exception/errinfo_api_function.hpp>
#include <boost/exception/errinfo_at_line.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/exception/errinfo_file_handle.hpp>
#include <boost/exception/errinfo_file_name.hpp>
#include <boost/exception/errinfo_file_open_mode.hpp>
#include <boost/exception/errinfo_type_info_name.hpp>
#include <boost/exception/info.hpp>
#include <boost/exception/get_error_info.hpp>
#include <boost/throw_exception.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <exception>

struct
test_exception:
    virtual boost::exception,
    virtual std::exception
    {
    };

int
main()
    {
    using namespace boost;
    try
        {
        test_exception e;
        e <<
            errinfo_api_function("failed_api_function") <<
            errinfo_at_line(42) <<
            errinfo_errno(0) <<
            errinfo_file_handle(weak_ptr<FILE>()) <<
            errinfo_file_name("filename.txt") <<
            errinfo_file_open_mode("rb");
#ifdef BOOST_NO_TYPEID
        BOOST_THROW_EXCEPTION(e);
#else
        BOOST_THROW_EXCEPTION(e<<errinfo_type_info_name(typeid(int).name()));
#endif
        BOOST_ERROR("BOOST_THROW_EXCEPTION failed to throw.");
        }
    catch(
    boost::exception & e )
        {
        BOOST_TEST(get_error_info<errinfo_api_function>(e) && *get_error_info<errinfo_api_function>(e)==std::string("failed_api_function"));
        BOOST_TEST(get_error_info<errinfo_at_line>(e) && *get_error_info<errinfo_at_line>(e)==42);
        BOOST_TEST(get_error_info<errinfo_errno>(e) && *get_error_info<errinfo_errno>(e)==0);
        BOOST_TEST(get_error_info<errinfo_file_handle>(e) && get_error_info<errinfo_file_handle>(e)->expired());
        BOOST_TEST(get_error_info<errinfo_file_name>(e) && *get_error_info<errinfo_file_name>(e)=="filename.txt");
        BOOST_TEST(get_error_info<errinfo_file_open_mode>(e) && *get_error_info<errinfo_file_open_mode>(e)=="rb");
#ifndef BOOST_NO_TYPEID
        BOOST_TEST(get_error_info<errinfo_type_info_name>(e) && *get_error_info<errinfo_type_info_name>(e)==typeid(int).name());
#endif
        }
    catch(
    ... )
        {
        BOOST_TEST(false);
        }
    return boost::report_errors();
    }
