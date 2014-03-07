//  (C) Copyright Gennadiy Rozental 2001-2008.
//  (C) Copyright Beman Dawes 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)


//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision: 49313 $
//
//  Description : tests an ability of Unit Test Framework to catch all kinds
//  of test errors in a user code and properly report it.
// ***************************************************************************

// Boost.Test
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/test/output_test_stream.hpp>
#include <boost/test/unit_test_log.hpp>
#include <boost/test/unit_test_suite.hpp>
#include <boost/test/framework.hpp>
#include <boost/test/detail/unit_test_parameters.hpp>
#include <boost/test/output/compiler_log_formatter.hpp>
#include <boost/test/results_reporter.hpp>

// STL
#include <iostream>
#include <stdexcept>

using namespace boost::unit_test;
using namespace boost::test_tools;

#if defined(__GNUC__) || defined(__SUNPRO_CC) || defined(__DECCXX_VER)
#define LIMITED_TEST
#endif

namespace {

struct this_test_log_formatter : public boost::unit_test::output::compiler_log_formatter
{
    void    print_prefix( std::ostream& output, boost::unit_test::const_string, std::size_t line )
    {
        output << line << ": ";
    }

    void test_unit_finish( std::ostream& output, test_unit const& tu, unsigned long )
    {
        output << "Leaving test " << tu.p_type_name << " \"" << tu.p_name << "\"" << std::endl;
    }

};

//____________________________________________________________________________//

char const* log_level_name[] = {
    "log_successful_tests",
    "log_test_suites",
    "log_messages",
    "log_warnings",
    "log_all_errors",
    "log_cpp_exception_errors",
    "log_system_errors",
    "log_fatal_errors",
    "log_nothing"
};

enum error_type_enum {
    et_begin,
    et_none = et_begin,
    et_message,
    et_warning,
    et_user,
    et_cpp_exception,
#ifdef LIMITED_TEST
    et_fatal_user,
#else
    et_system,
    et_fatal_user,
    et_fatal_system,
#endif
    et_end
} error_type;

char const* error_type_name[] = {
    "no error", "user message", "user warning",
    "user non-fatal error", "cpp exception", " system error",
    "user fatal error", "system fatal error"
};

int divide_by_zero = 0;

void error_on_demand()
{
    switch( error_type ) {
    case et_none:
        BOOST_CHECK_MESSAGE( divide_by_zero == 0, "no error" );
        break;

    case et_message:
        BOOST_TEST_MESSAGE( "message" );
        break;

    case et_warning:
        BOOST_WARN_MESSAGE( divide_by_zero != 0, "warning" );
        break;

    case et_user:
        BOOST_ERROR( "non-fatal error" );
        break;

    case et_fatal_user:
        BOOST_FAIL( "fatal error" );

        BOOST_ERROR( "Should never reach this code!" );
        break;

    case et_cpp_exception:
        BOOST_TEST_CHECKPOINT( "error_on_demand() throw runtime_error" );
        throw std::runtime_error( "test std::runtime error what() message" );

#ifndef LIMITED_TEST
    case et_system:
        BOOST_TEST_CHECKPOINT( "error_on_demand() divide by zero" );
        divide_by_zero = 1 / divide_by_zero;
        break;

    case et_fatal_system:
        BOOST_TEST_CHECKPOINT( "write to an invalid address" );
        {
            int* p = 0;
            *p = 0;

            BOOST_ERROR( "Should never reach this code!" );
        }
        break;
#endif
    default:
        BOOST_ERROR( "Should never reach this code!" );
    }
    return;
}

}  // local namespace

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test_errors_handling )
{
#define PATTERN_FILE_NAME "errors_handling_test.pattern"
    std::string pattern_file_name(
        framework::master_test_suite().argc <= 1 
            ? (runtime_config::save_pattern() ? PATTERN_FILE_NAME : "./test_files/" PATTERN_FILE_NAME)
            : framework::master_test_suite().argv[1] );

#ifdef LIMITED_TEST
    pattern_file_name += "2";
#endif

    output_test_stream test_output( pattern_file_name, !runtime_config::save_pattern() );

    test_case* test = BOOST_TEST_CASE( &error_on_demand );

    // for each log level
    for( log_level level = log_successful_tests;
         level          <= log_nothing;
         level           = static_cast<log_level>(level+1) )
    {
        // for each error type
        for( error_type = et_begin;
             error_type != et_end;
             error_type = static_cast<error_type_enum>(error_type+1) )
        {
            test_output << "\n===========================\n"
                        << "log level: "       << log_level_name[level] << ';'
                        << " error type: "     << error_type_name[error_type] << ";\n" << std::endl;

            unit_test_log.set_stream( test_output );
            unit_test_log.set_formatter( new this_test_log_formatter );
            unit_test_log.set_threshold_level( level );
            framework::run( test );

            unit_test_log.set_stream( std::cout );
            unit_test_log.set_format( runtime_config::log_format() );
            unit_test_log.set_threshold_level( runtime_config::log_level() != invalid_log_level
                                                ? runtime_config::log_level()
                                                : log_all_errors );
            BOOST_CHECK( test_output.match_pattern() );
        }
    }
}

//____________________________________________________________________________//

// EOF

