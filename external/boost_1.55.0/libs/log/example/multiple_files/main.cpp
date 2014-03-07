/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   main.cpp
 * \author Andrey Semashev
 * \date   26.04.2008
 *
 * \brief  This example shows how to perform logging to several files simultaneously,
 *         with files being created on an attribute value basis - thread identifier in this case.
 *         In the example the application creates a number of threads and registers thread
 *         identifiers as attributes. Every thread performs logging, and the sink separates
 *         log records from different threads into different files.
 */

// #define BOOST_LOG_DYN_LINK 1

#include <stdexcept>
#include <string>
#include <iostream>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_multifile_backend.hpp>

namespace logging = boost::log;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;

using boost::shared_ptr;

enum
{
    THREAD_COUNT = 5,
    LOG_RECORDS_TO_WRITE = 10
};

// Global logger declaration
BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(my_logger, src::logger_mt)

// This function is executed in a separate thread
void thread_foo()
{
    BOOST_LOG_SCOPED_THREAD_TAG("ThreadID", boost::this_thread::get_id());
    for (unsigned int i = 0; i < LOG_RECORDS_TO_WRITE; ++i)
    {
        BOOST_LOG(my_logger::get()) << "Log record " << i;
    }
}

int main(int argc, char* argv[])
{
    try
    {
        // Create a text file sink
        typedef sinks::synchronous_sink< sinks::text_multifile_backend > file_sink;
        shared_ptr< file_sink > sink(new file_sink);

        // Set up how the file names will be generated
        sink->locked_backend()->set_file_name_composer(sinks::file::as_file_name_composer(
            expr::stream << "logs/" << expr::attr< boost::thread::id >("ThreadID") << ".log"));

        // Set the log record formatter
        sink->set_formatter
        (
            expr::format("%1%: [%2%] - %3%")
                % expr::attr< unsigned int >("RecordID")
                % expr::attr< boost::posix_time::ptime >("TimeStamp")
                % expr::smessage
        );

        // Add it to the core
        logging::core::get()->add_sink(sink);

        // Add some attributes too
        logging::core::get()->add_global_attribute("TimeStamp", attrs::local_clock());
        logging::core::get()->add_global_attribute("RecordID", attrs::counter< unsigned int >());

        // Create threads and make some logs
        boost::thread_group threads;
        for (unsigned int i = 0; i < THREAD_COUNT; ++i)
            threads.create_thread(&thread_foo);

        threads.join_all();

        return 0;
    }
    catch (std::exception& e)
    {
        std::cout << "FAILURE: " << e.what() << std::endl;
        return 1;
    }
}
