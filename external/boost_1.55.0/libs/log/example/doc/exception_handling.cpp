/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include <cstddef>
#include <string>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sources/basic_logger.hpp>
#include <boost/log/sources/severity_feature.hpp>
#include <boost/log/sources/exception_handler_feature.hpp>
#include <boost/log/sources/features.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>
#include <boost/log/utility/exception_handler.hpp>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace attrs = boost::log::attributes;
namespace keywords = boost::log::keywords;

//[ example_sources_exception_handler
enum severity_level
{
    normal,
    warning,
    error
};

// A logger class that allows to intercept exceptions and supports severity level
class my_logger_mt :
    public src::basic_composite_logger<
        char,
        my_logger_mt,
        src::multi_thread_model< boost::shared_mutex >,
        src::features<
            src::severity< severity_level >,
            src::exception_handler
        >
    >
{
    BOOST_LOG_FORWARD_LOGGER_MEMBERS(my_logger_mt)
};

BOOST_LOG_INLINE_GLOBAL_LOGGER_INIT(my_logger, my_logger_mt)
{
    my_logger_mt lg;

    // Set up exception handler: all exceptions that occur while
    // logging through this logger, will be suppressed
    lg.set_exception_handler(logging::make_exception_suppressor());

    return lg;
}

void logging_function()
{
    // This will not throw
    BOOST_LOG_SEV(my_logger::get(), normal) << "Hello, world";
}
//]

//[ example_utility_exception_handler
struct my_handler
{
    typedef void result_type;

    void operator() (std::runtime_error const& e) const
    {
        std::cout << "std::runtime_error: " << e.what() << std::endl;
    }
    void operator() (std::logic_error const& e) const
    {
        std::cout << "std::logic_error: " << e.what() << std::endl;
        throw;
    }
};

void init_exception_handler()
{
    // Setup a global exception handler that will call my_handler::operator()
    // for the specified exception types
    logging::core::get()->set_exception_handler(logging::make_exception_handler<
        std::runtime_error,
        std::logic_error
    >(my_handler()));
}
//]

//[ example_utility_exception_handler_nothrow
struct my_handler_nothrow
{
    typedef void result_type;

    void operator() (std::runtime_error const& e) const
    {
        std::cout << "std::runtime_error: " << e.what() << std::endl;
    }
    void operator() (std::logic_error const& e) const
    {
        std::cout << "std::logic_error: " << e.what() << std::endl;
        throw;
    }
    void operator() () const
    {
        std::cout << "unknown exception" << std::endl;
    }
};

void init_exception_handler_nothrow()
{
    // Setup a global exception handler that will call my_handler::operator()
    // for the specified exception types. Note the std::nothrow argument that
    // specifies that all other exceptions should also be passed to the functor.
    logging::core::get()->set_exception_handler(logging::make_exception_handler<
        std::runtime_error,
        std::logic_error
    >(my_handler_nothrow(), std::nothrow));
}
//]

void init()
{
    typedef sinks::synchronous_sink< sinks::text_ostream_backend > text_sink;
    boost::shared_ptr< text_sink > sink = boost::make_shared< text_sink >();

    sink->locked_backend()->add_stream(
        boost::make_shared< std::ofstream >("sample.log"));

    sink->set_formatter
    (
        expr::stream
            << expr::attr< unsigned int >("LineID")
            << ": <" << expr::attr< severity_level >("Severity")
            << "> " << expr::smessage
    );

    logging::core::get()->add_sink(sink);

    init_exception_handler();
}

int main(int, char*[])
{
    init();
    logging_function();

    return 0;
}
