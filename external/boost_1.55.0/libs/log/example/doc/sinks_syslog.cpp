/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include <string>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>
#include <boost/log/core.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/syslog_backend.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;

//[ example_sinks_syslog
// Complete sink type
typedef sinks::synchronous_sink< sinks::syslog_backend > sink_t;

//<-
#if defined(BOOST_LOG_USE_NATIVE_SYSLOG)
//->
void init_native_syslog()
{
    boost::shared_ptr< logging::core > core = logging::core::get();

    // Create a backend
    boost::shared_ptr< sinks::syslog_backend > backend(new sinks::syslog_backend(
        keywords::facility = sinks::syslog::user,               /*< the logging facility >*/
        keywords::use_impl = sinks::syslog::native              /*< the native syslog API should be used >*/
    ));

    // Set the straightforward level translator for the "Severity" attribute of type int
    backend->set_severity_mapper(sinks::syslog::direct_severity_mapping< int >("Severity"));

    // Wrap it into the frontend and register in the core.
    // The backend requires synchronization in the frontend.
    core->add_sink(boost::make_shared< sink_t >(backend));
}
//<-
#endif
//->

//<-
#if !defined(BOOST_LOG_NO_ASIO)
//->
void init_builtin_syslog()
{
    boost::shared_ptr< logging::core > core = logging::core::get();

    // Create a new backend
    boost::shared_ptr< sinks::syslog_backend > backend(new sinks::syslog_backend(
        keywords::facility = sinks::syslog::local0,             /*< the logging facility >*/
        keywords::use_impl = sinks::syslog::udp_socket_based    /*< the built-in socket-based implementation should be used >*/
    ));

    // Setup the target address and port to send syslog messages to
    backend->set_target_address("192.164.1.10", 514);

    // Create and fill in another level translator for "MyLevel" attribute of type string
    sinks::syslog::custom_severity_mapping< std::string > mapping("MyLevel");
    mapping["debug"] = sinks::syslog::debug;
    mapping["normal"] = sinks::syslog::info;
    mapping["warning"] = sinks::syslog::warning;
    mapping["failure"] = sinks::syslog::critical;
    backend->set_severity_mapper(mapping);

    // Wrap it into the frontend and register in the core.
    core->add_sink(boost::make_shared< sink_t >(backend));
}
//<-
#endif
//->
//]

int main(int, char*[])
{
#if defined(BOOST_LOG_USE_NATIVE_SYSLOG)
    init_native_syslog();
#elif !defined(BOOST_LOG_NO_ASIO)
    init_builtin_syslog();
#endif

    BOOST_LOG_SCOPED_THREAD_TAG("MyLevel", "warning");
    src::severity_logger< > lg;
    BOOST_LOG_SEV(lg, 3) << "Hello world!";

    return 0;
}
