/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include <cstddef>
#include <string>
#include <ostream>
#include <fstream>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace attrs = boost::log::attributes;
namespace keywords = boost::log::keywords;

//[ example_sources_severity_channel
enum severity_level
{
    normal,
    notification,
    warning,
    error,
    critical
};

typedef src::severity_channel_logger_mt<
    severity_level,     // the type of the severity level
    std::string         // the type of the channel name
> my_logger_mt;


BOOST_LOG_INLINE_GLOBAL_LOGGER_INIT(my_logger, my_logger_mt)
{
    // Specify the channel name on construction, similarly as with the channel_logger
    return my_logger_mt(keywords::channel = "my_logger");
}

void logging_function()
{
    // Do logging with the severity level. The record will have both
    // the severity level and the channel name attached.
    BOOST_LOG_SEV(my_logger::get(), normal) << "Hello, world!";
}
//]

// The operator puts a human-friendly representation of the severity level to the stream
std::ostream& operator<< (std::ostream& strm, severity_level level)
{
    static const char* strings[] =
    {
        "normal",
        "notification",
        "warning",
        "error",
        "critical"
    };

    if (static_cast< std::size_t >(level) < sizeof(strings) / sizeof(*strings))
        strm << strings[level];
    else
        strm << static_cast< int >(level);

    return strm;
}

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
            << ">\t"
            << "[" << expr::attr< std::string >("Channel") << "] "
            << expr::smessage
    );

    logging::core::get()->add_sink(sink);

    // Add attributes
    logging::add_common_attributes();
}

int main(int, char*[])
{
    init();
    logging_function();

    return 0;
}
