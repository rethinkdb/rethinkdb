/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include <cstddef>
#include <iostream>
#include <boost/log/expressions.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/log/utility/manipulators/to_log.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;

//[ example_expressions_attr_formatter_stream_tag
// We define our own severity levels
enum severity_level
{
    normal,
    notification,
    warning,
    error,
    critical
};

// The operator is used for regular stream formatting
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

// Attribute value tag type
struct severity_tag;

// The operator is used when putting the severity level to log
logging::formatting_ostream& operator<<
(
    logging::formatting_ostream& strm,
    logging::to_log_manip< severity_level, severity_tag > const& manip
)
{
    static const char* strings[] =
    {
        "NORM",
        "NTFY",
        "WARN",
        "ERRR",
        "CRIT"
    };

    severity_level level = manip.get();
    if (static_cast< std::size_t >(level) < sizeof(strings) / sizeof(*strings))
        strm << strings[level];
    else
        strm << static_cast< int >(level);

    return strm;
}

void init()
{
    logging::add_console_log
    (
        std::clog,
        // This makes the sink to write log records that look like this:
        // 1: <NORM> A normal severity message
        // 2: <ERRR> An error severity message
        keywords::format =
        (
            expr::stream
                << expr::attr< unsigned int >("LineID")
                << ": <" << expr::attr< severity_level, severity_tag >("Severity")
                << "> " << expr::smessage
        )
    );
}
//]

int main(int, char*[])
{
    init();
    logging::add_common_attributes();

    src::severity_logger< severity_level > lg;

    // These messages will be written with CAPS severity levels
    BOOST_LOG_SEV(lg, normal) << "A normal severity message";
    BOOST_LOG_SEV(lg, notification) << "A notification severity message";
    BOOST_LOG_SEV(lg, warning) << "A warning severity message";
    BOOST_LOG_SEV(lg, error) << "An error severity message";
    BOOST_LOG_SEV(lg, critical) << "A critical severity message";

    // This line will still use lower-case severity levels
    std::cout << "The regular output still uses lower-case formatting: " << normal << std::endl;

    return 0;
}
