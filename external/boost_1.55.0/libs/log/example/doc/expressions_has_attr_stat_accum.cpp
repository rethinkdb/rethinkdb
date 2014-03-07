/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include <cstddef>
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/utility/value_ref.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;

//[ example_expressions_has_attr_stat_accumulator
// Declare attribute keywords
BOOST_LOG_ATTRIBUTE_KEYWORD(stat_stream, "StatisticStream", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(change, "Change", int)

// A simple sink backend to accumulate statistic information
class my_stat_accumulator :
    public sinks::basic_sink_backend< sinks::synchronized_feeding >
{
    // A map of accumulated statistic values,
    // ordered by the statistic information stream name
    typedef std::map< std::string, int > stat_info_map;
    stat_info_map m_stat_info;

public:
    // Destructor
    ~my_stat_accumulator()
    {
        // Display the accumulated data
        stat_info_map::const_iterator it = m_stat_info.begin(), end = m_stat_info.end();
        for (; it != end; ++it)
        {
            std::cout << "Statistic stream: " << it->first
                << ", accumulated value: " << it->second << "\n";
        }
        std::cout.flush();
    }

    // The method is called for every log record being put into the sink backend
    void consume(logging::record_view const& rec)
    {
        // First, acquire statistic information stream name
        logging::value_ref< std::string, tag::stat_stream > name = rec[stat_stream];
        if (name)
        {
            // Next, get the statistic value change
            logging::value_ref< int, tag::change > change_amount = rec[change];
            if (change_amount)
            {
                // Accumulate the statistic data
                m_stat_info[name.get()] += change_amount.get();
            }
        }
    }
};

// The function registers two sinks - one for statistic information,
// and another one for other records
void init()
{
    boost::shared_ptr< logging::core > core = logging::core::get();

    // Create a backend and attach a stream to it
    boost::shared_ptr< sinks::text_ostream_backend > backend =
        boost::make_shared< sinks::text_ostream_backend >();
    backend->add_stream(
        boost::shared_ptr< std::ostream >(new std::ofstream("test.log")));

    // Create a frontend and setup filtering
    typedef sinks::synchronous_sink< sinks::text_ostream_backend > log_sink_type;
    boost::shared_ptr< log_sink_type > log_sink(new log_sink_type(backend));
    // All records that don't have a "StatisticStream" attribute attached
    // will go to the "test.log" file
    log_sink->set_filter(!expr::has_attr(stat_stream));

    core->add_sink(log_sink);

    // Create another sink that will receive all statistic data
    typedef sinks::synchronous_sink< my_stat_accumulator > stat_sink_type;
    boost::shared_ptr< stat_sink_type > stat_sink(new stat_sink_type());
    // All records with a "StatisticStream" string attribute attached
    // will go to the my_stat_accumulator sink
    stat_sink->set_filter(expr::has_attr(stat_stream));

    core->add_sink(stat_sink);
}

// This simple macro will simplify putting statistic data into a logger
#define PUT_STAT(lg, stat_stream_name, change)\
    if (true) {\
        BOOST_LOG_SCOPED_LOGGER_TAG(lg, "StatisticStream", stat_stream_name);\
        BOOST_LOG(lg) << logging::add_value("Change", (int)(change));\
    } else ((void)0)

void logging_function()
{
    src::logger lg;

    // Put a regular log record, it will go to the "test.log" file
    BOOST_LOG(lg) << "A regular log record";

    // Put some statistic data
    PUT_STAT(lg, "StreamOne", 10);
    PUT_STAT(lg, "StreamTwo", 20);
    PUT_STAT(lg, "StreamOne", -5);
}
//]

int main(int, char*[])
{
    init();
    logging_function();

    return 0;
}
