/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include <string>
#include <fstream>
#include <iostream>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/utility/empty_deleter.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;

//[ example_sinks_sync
enum severity_level
{
    normal,
    warning,
    error
};

// Complete sink type
typedef sinks::synchronous_sink< sinks::text_ostream_backend > sink_t;

void init_logging()
{
    boost::shared_ptr< logging::core > core = logging::core::get();

    // Create a backend and initialize it with a stream
    boost::shared_ptr< sinks::text_ostream_backend > backend =
        boost::make_shared< sinks::text_ostream_backend >();
    backend->add_stream(
        boost::shared_ptr< std::ostream >(&std::clog, boost::empty_deleter()));

    // Wrap it into the frontend and register in the core
    boost::shared_ptr< sink_t > sink(new sink_t(backend));
    core->add_sink(sink);

    // You can manage filtering and formatting through the sink interface
    sink->set_filter(expr::attr< severity_level >("Severity") >= warning);
    sink->set_formatter
    (
        expr::stream
            << "Level: " << expr::attr< severity_level >("Severity")
            << " Message: " << expr::smessage
    );

    // You can also manage backend in a thread-safe manner
    {
        sink_t::locked_backend_ptr p = sink->locked_backend();
        p->add_stream(boost::make_shared< std::ofstream >("sample.log"));
    } // the backend gets released here
}
//]

int main(int, char*[])
{
    init_logging();

    src::severity_channel_logger< severity_level > lg(keywords::channel = "net");
    BOOST_LOG_SEV(lg, warning) << "Hello world!";

    return 0;
}
