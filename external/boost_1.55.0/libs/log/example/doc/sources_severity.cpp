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
#include <boost/move/utility.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace attrs = boost::log::attributes;
namespace keywords = boost::log::keywords;

//[ example_sources_severity
// We define our own severity levels
enum severity_level
{
    normal,
    notification,
    warning,
    error,
    critical
};

void logging_function()
{
    // The logger implicitly adds a source-specific attribute 'Severity'
    // of type 'severity_level' on construction
    src::severity_logger< severity_level > slg;

    BOOST_LOG_SEV(slg, normal) << "A regular message";
    BOOST_LOG_SEV(slg, warning) << "Something bad is going on but I can handle it";
    BOOST_LOG_SEV(slg, critical) << "Everything crumbles, shoot me now!";
}
//]

//[ example_sources_default_severity
void default_severity()
{
    // The default severity can be specified in constructor.
    src::severity_logger< severity_level > error_lg(keywords::severity = error);

    BOOST_LOG(error_lg) << "An error level log record (by default)";

    // The explicitly specified level overrides the default
    BOOST_LOG_SEV(error_lg, warning) << "A warning level log record (overrode the default)";
}
//]

//[ example_sources_severity_manual
void manual_logging()
{
    src::severity_logger< severity_level > slg;

    logging::record rec = slg.open_record(keywords::severity = normal);
    if (rec)
    {
        logging::record_ostream strm(rec);
        strm << "A regular message";
        strm.flush();
        slg.push_record(boost::move(rec));
    }
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
    default_severity();
    manual_logging();

    return 0;
}
