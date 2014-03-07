/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions/predicates/is_debugger_present.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/debug_output_backend.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

#if defined(BOOST_WINDOWS)

namespace logging = boost::log;
namespace expr = boost::log::expressions;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;

//[ example_sinks_debugger
// Complete sink type
typedef sinks::synchronous_sink< sinks::debug_output_backend > sink_t;

void init_logging()
{
    boost::shared_ptr< logging::core > core = logging::core::get();

    // Create the sink. The backend requires synchronization in the frontend.
    boost::shared_ptr< sink_t > sink(new sink_t());

    // Set the special filter to the frontend
    // in order to skip the sink when no debugger is available
    sink->set_filter(expr::is_debugger_present());

    core->add_sink(sink);
}
//]

int main(int, char*[])
{
    init_logging();

    src::logger lg;
    BOOST_LOG(lg) << "Hello world!";

    return 0;
}

#else

int main(int, char*[])
{
    return 0;
}

#endif
