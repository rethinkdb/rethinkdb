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
#include <boost/log/core.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;

//[ example_sinks_file
void init_logging()
{
    boost::shared_ptr< logging::core > core = logging::core::get();

    boost::shared_ptr< sinks::text_file_backend > backend =
        boost::make_shared< sinks::text_file_backend >(
            keywords::file_name = "file_%5N.log",                                          /*< file name pattern >*/
            keywords::rotation_size = 5 * 1024 * 1024,                                     /*< rotate the file upon reaching 5 MiB size... >*/
            keywords::time_based_rotation = sinks::file::rotation_at_time_point(12, 0, 0)  /*< ...or every day, at noon, whichever comes first >*/
        );

    // Wrap it into the frontend and register in the core.
    // The backend requires synchronization in the frontend.
    typedef sinks::synchronous_sink< sinks::text_file_backend > sink_t;
    boost::shared_ptr< sink_t > sink(new sink_t(backend));

    core->add_sink(sink);
}
//]

int main(int, char*[])
{
    init_logging();

    src::severity_channel_logger< > lg(keywords::channel = "net");
    BOOST_LOG_SEV(lg, 3) << "Hello world!";

    return 0;
}
