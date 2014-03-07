/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include <stdexcept>
#include <string>
#include <iostream>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/sources/logger.hpp>

namespace logging = boost::log;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;

typedef sinks::synchronous_sink< sinks::text_file_backend > file_sink;

//[ example_sinks_xml_file_collecting
void init_file_collecting(boost::shared_ptr< file_sink > sink)
{
    sink->locked_backend()->set_file_collector(sinks::file::make_collector(
        keywords::target = "logs",                      /*< the target directory >*/
        keywords::max_size = 16 * 1024 * 1024,          /*< maximum total size of the stored files, in bytes >*/
        keywords::min_free_space = 100 * 1024 * 1024    /*< minimum free space on the drive, in bytes >*/
    ));
}
//]

#if 0
//[ example_sinks_xml_file
// Complete file sink type
typedef sinks::synchronous_sink< sinks::text_file_backend > file_sink;

void write_header(sinks::text_file_backend::stream_type& file)
{
    file << "<?xml version=\"1.0\"?>\n<log>\n";
}

void write_footer(sinks::text_file_backend::stream_type& file)
{
    file << "</log>\n";
}

void init_logging()
{
    // Create a text file sink
    boost::shared_ptr< file_sink > sink(new file_sink(
        keywords::file_name = "%Y%m%d_%H%M%S_%5N.xml",  /*< the resulting file name pattern >*/
        keywords::rotation_size = 16384                 /*< rotation size, in characters >*/
    ));

    sink->set_formatter
    (
        expr::format("\t<record id=\"%1%\" timestamp=\"%2%\">%3%</record>")
            % expr::attr< unsigned int >("RecordID")
            % expr::attr< boost::posix_time::ptime >("TimeStamp")
            % expr::xml_decor[ expr::stream << expr::smessage ]            /*< the log message has to be decorated, if it contains special characters >*/
    );

    // Set header and footer writing functors
    sink->locked_backend()->set_open_handler(&write_header);
    sink->locked_backend()->set_close_handler(&write_footer);

    // Add the sink to the core
    logging::core::get()->add_sink(sink);
}
//]
#endif

//[ example_sinks_xml_file_final
void init_logging()
{
    // Create a text file sink
    boost::shared_ptr< file_sink > sink(new file_sink(
        keywords::file_name = "%Y%m%d_%H%M%S_%5N.xml",
        keywords::rotation_size = 16384
    ));

    // Set up where the rotated files will be stored
    init_file_collecting(sink);

    // Upon restart, scan the directory for files matching the file_name pattern
    sink->locked_backend()->scan_for_files();

    sink->set_formatter
    (
        expr::format("\t<record id=\"%1%\" timestamp=\"%2%\">%3%</record>")
            % expr::attr< unsigned int >("RecordID")
            % expr::attr< boost::posix_time::ptime >("TimeStamp")
            % expr::xml_decor[ expr::stream << expr::smessage ]
    );

    // Set header and footer writing functors
    namespace bll = boost::lambda;

    sink->locked_backend()->set_open_handler
    (
        bll::_1 << "<?xml version=\"1.0\"?>\n<log>\n"
    );
    sink->locked_backend()->set_close_handler
    (
        bll::_1 << "</log>\n"
    );

    // Add the sink to the core
    logging::core::get()->add_sink(sink);
}
//]

enum { LOG_RECORDS_TO_WRITE = 2000 };

int main(int argc, char* argv[])
{
    try
    {
        // Initialize logging library
        init_logging();

        // And also add some attributes
        logging::core::get()->add_global_attribute("TimeStamp", attrs::local_clock());
        logging::core::get()->add_global_attribute("RecordID", attrs::counter< unsigned int >());

        // Do some logging
        src::logger lg;
        for (unsigned int i = 0; i < LOG_RECORDS_TO_WRITE; ++i)
        {
            BOOST_LOG(lg) << "XML log record " << i;
        }

        // Test that XML character decoration works
        BOOST_LOG(lg) << "Special XML characters: &, <, >, '";

        return 0;
    }
    catch (std::exception& e)
    {
        std::cout << "FAILURE: " << e.what() << std::endl;
        return 1;
    }
}
