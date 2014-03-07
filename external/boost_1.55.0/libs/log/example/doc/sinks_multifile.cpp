/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include <string>
#include <iostream>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_multifile_backend.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;

//[ example_sinks_multifile
void init_logging()
{
    boost::shared_ptr< logging::core > core = logging::core::get();

    boost::shared_ptr< sinks::text_multifile_backend > backend =
        boost::make_shared< sinks::text_multifile_backend >();

    // Set up the file naming pattern
    backend->set_file_name_composer
    (
        sinks::file::as_file_name_composer(expr::stream << "logs/" << expr::attr< std::string >("RequestID") << ".log")
    );

    // Wrap it into the frontend and register in the core.
    // The backend requires synchronization in the frontend.
    typedef sinks::synchronous_sink< sinks::text_multifile_backend > sink_t;
    boost::shared_ptr< sink_t > sink(new sink_t(backend));

    // Set the formatter
    sink->set_formatter
    (
        expr::stream
            << "[RequestID: " << expr::attr< std::string >("RequestID")
            << "] " << expr::smessage
    );

    core->add_sink(sink);
}
//]

void logging_function()
{
    src::logger lg;
    BOOST_LOG(lg) << "Hello, world!";
}

int main(int, char*[])
{
    init_logging();

    {
        BOOST_LOG_SCOPED_THREAD_TAG("RequestID", "Request1");
        logging_function();
    }
    {
        BOOST_LOG_SCOPED_THREAD_TAG("RequestID", "Request2");
        logging_function();
    }

    return 0;
}
