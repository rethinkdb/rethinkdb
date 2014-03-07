/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   main.cpp
 * \author Andrey Semashev
 * \date   07.03.2009
 *
 * \brief  An example of logging to a syslog server (syslogd, for example).
 *
 * The example shows how to initialize logging to a remote syslog server.
 * The code creates a sink that will send syslog messages to local port 514.
 */

// #define BOOST_LOG_DYN_LINK 1

#include <stdexcept>
#include <string>
#include <iostream>
#include <boost/smart_ptr/shared_ptr.hpp>

#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/syslog_backend.hpp>

namespace logging = boost::log;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;

using boost::shared_ptr;

//! Define application-specific severity levels
enum severity_levels
{
    normal,
    warning,
    error
};

int main(int argc, char* argv[])
{
    try
    {
        // Create a syslog sink
        shared_ptr< sinks::synchronous_sink< sinks::syslog_backend > > sink(
            new sinks::synchronous_sink< sinks::syslog_backend >());

        sink->set_formatter
        (
            expr::format("syslog.exe: %1%: %2%")
                % expr::attr< unsigned int >("RecordID")
                % expr::smessage
        );

        // We'll have to map our custom levels to the syslog levels
        sinks::syslog::custom_severity_mapping< severity_levels > mapping("Severity");
        mapping[normal] = sinks::syslog::info;
        mapping[warning] = sinks::syslog::warning;
        mapping[error] = sinks::syslog::critical;

        sink->locked_backend()->set_severity_mapper(mapping);

        // Set the remote address to sent syslog messages to
        sink->locked_backend()->set_target_address("localhost");

        // Add the sink to the core
        logging::core::get()->add_sink(sink);

        // Add some attributes too
        logging::core::get()->add_global_attribute("RecordID", attrs::counter< unsigned int >());

        // Do some logging
        src::severity_logger< severity_levels > lg(keywords::severity = normal);
        BOOST_LOG_SEV(lg, normal) << "A syslog record with normal level";
        BOOST_LOG_SEV(lg, warning) << "A syslog record with warning level";
        BOOST_LOG_SEV(lg, error) << "A syslog record with error level";

        return 0;
    }
    catch (std::exception& e)
    {
        std::cout << "FAILURE: " << e.what() << std::endl;
        return 1;
    }
}
