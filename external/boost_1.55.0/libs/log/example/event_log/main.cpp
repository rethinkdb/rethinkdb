/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   main.cpp
 * \author Andrey Semashev
 * \date   16.11.2008
 *
 * \brief  An example of logging into Windows event log.
 *
 * The example shows the basic usage of the Windows NT event log backend.
 * The code defines custom severity levels, initializes the sink and a couple of
 * attributes to test with, and writes several records at different levels.
 * As a result the written records should appear in the Application log, and
 * should be displayed correctly with the Windows event log viewer.
 */

#include <stdexcept>
#include <string>
#include <iostream>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <boost/log/common.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/event_log_backend.hpp>

#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include "event_log_messages.h"

namespace logging = boost::log;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;

//[ example_sinks_event_log_severity
// Define application-specific severity levels
enum severity_level
{
    normal,
    warning,
    error
};
//]

void init_logging()
{
    //[ example_sinks_event_log_create_backend
    // Create an event log sink
    boost::shared_ptr< sinks::event_log_backend > backend(
        new sinks::event_log_backend((
            keywords::message_file = "%SystemDir%\\event_log_messages.dll",
            keywords::log_name = "My Application",
            keywords::log_source = "My Source"
        ))
    );
    //]

    //[ example_sinks_event_log_event_composer
    // Create an event composer. It is initialized with the event identifier mapping.
    sinks::event_log::event_composer composer(
        sinks::event_log::direct_event_id_mapping< int >("EventID"));

    // For each event described in the message file, set up the insertion string formatters
    composer[LOW_DISK_SPACE_MSG]
        // the first placeholder in the message
        // will be replaced with contents of the "Drive" attribute
        % expr::attr< std::string >("Drive")
        // the second placeholder in the message
        // will be replaced with contents of the "Size" attribute
        % expr::attr< boost::uintmax_t >("Size");

    composer[DEVICE_INACCESSIBLE_MSG]
        % expr::attr< std::string >("Drive");

    composer[SUCCEEDED_MSG]
        % expr::attr< unsigned int >("Duration");

    // Then put the composer to the backend
    backend->set_event_composer(composer);
    //]

    //[ example_sinks_event_log_mappings
    // We'll have to map our custom levels to the event log event types
    sinks::event_log::custom_event_type_mapping< severity_level > type_mapping("Severity");
    type_mapping[normal] = sinks::event_log::make_event_type(MY_SEVERITY_INFO);
    type_mapping[warning] = sinks::event_log::make_event_type(MY_SEVERITY_WARNING);
    type_mapping[error] = sinks::event_log::make_event_type(MY_SEVERITY_ERROR);

    backend->set_event_type_mapper(type_mapping);

    // Same for event categories.
    // Usually event categories can be restored by the event identifier.
    sinks::event_log::custom_event_category_mapping< int > cat_mapping("EventID");
    cat_mapping[LOW_DISK_SPACE_MSG] = sinks::event_log::make_event_category(MY_CATEGORY_1);
    cat_mapping[DEVICE_INACCESSIBLE_MSG] = sinks::event_log::make_event_category(MY_CATEGORY_2);
    cat_mapping[SUCCEEDED_MSG] = sinks::event_log::make_event_category(MY_CATEGORY_3);

    backend->set_event_category_mapper(cat_mapping);
    //]

    //[ example_sinks_event_log_register_sink
    // Create the frontend for the sink
    boost::shared_ptr< sinks::synchronous_sink< sinks::event_log_backend > > sink(
        new sinks::synchronous_sink< sinks::event_log_backend >(backend));

    // Set up filter to pass only records that have the necessary attribute
    sink->set_filter(expr::has_attr< int >("EventID"));

    logging::core::get()->add_sink(sink);
    //]
}

//[ example_sinks_event_log_facilities
BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(event_logger, src::severity_logger_mt< severity_level >)

// The function raises an event of the disk space depletion
void announce_low_disk_space(std::string const& drive, boost::uintmax_t size)
{
    BOOST_LOG_SCOPED_THREAD_TAG("EventID", (int)LOW_DISK_SPACE_MSG);
    BOOST_LOG_SCOPED_THREAD_TAG("Drive", drive);
    BOOST_LOG_SCOPED_THREAD_TAG("Size", size);
    // Since this record may get accepted by other sinks,
    // this message is not completely useless
    BOOST_LOG_SEV(event_logger::get(), warning) << "Low disk " << drive
        << " space, " << size << " Mb is recommended";
}

// The function raises an event of inaccessible disk drive
void announce_device_inaccessible(std::string const& drive)
{
    BOOST_LOG_SCOPED_THREAD_TAG("EventID", (int)DEVICE_INACCESSIBLE_MSG);
    BOOST_LOG_SCOPED_THREAD_TAG("Drive", drive);
    BOOST_LOG_SEV(event_logger::get(), error) << "Cannot access drive " << drive;
}

// The structure is an activity guard that will emit an event upon the activity completion
struct activity_guard
{
    activity_guard()
    {
        // Add a stop watch attribute to measure the activity duration
        m_it = event_logger::get().add_attribute("Duration", attrs::timer()).first;
    }
    ~activity_guard()
    {
        BOOST_LOG_SCOPED_THREAD_TAG("EventID", (int)SUCCEEDED_MSG);
        BOOST_LOG_SEV(event_logger::get(), normal) << "Activity ended";
        event_logger::get().remove_attribute(m_it);
    }

private:
    logging::attribute_set::iterator m_it;
};
//]

int main(int argc, char* argv[])
{
    try
    {
        // Initialize the library
        init_logging();

        // Make some events
        {
            activity_guard activity;

            announce_low_disk_space("C:", 2 * 1024 * 1024);
            announce_device_inaccessible("D:");
        }

        return 0;
    }
    catch (std::exception& e)
    {
        std::cout << "FAILURE: " << e.what() << std::endl;
        return 1;
    }
}
