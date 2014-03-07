/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional/optional.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/phoenix.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/basic_sink_backend.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/attributes/value_visitation.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/utility/setup/filter_parser.hpp>
#include <boost/log/utility/setup/from_stream.hpp>
#include <boost/log/utility/setup/from_settings.hpp>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;

//[ example_extension_stat_collector_settings_definition
// The backend collects statistical information about network activity of the application
class stat_collector :
    public sinks::basic_sink_backend<
        sinks::combine_requirements<
            sinks::synchronized_feeding,
            sinks::flushing
        >::type
    >
{
private:
    // The file to write the collected information to
    std::ofstream m_csv_file;

    // Here goes the data collected so far:
    // Active connections
    unsigned int m_active_connections;
    // Sent bytes
    unsigned int m_sent_bytes;
    // Received bytes
    unsigned int m_received_bytes;

    // The number of collected records since the last write to the file
    unsigned int m_collected_count;
    // The time when the collected data has been written to the file last time
    boost::posix_time::ptime m_last_store_time;
    // The collected data writing interval
    boost::posix_time::time_duration m_write_interval;

public:
    // The constructor initializes the internal data
    stat_collector(const char* file_name, boost::posix_time::time_duration write_interval);

    // The function consumes the log records that come from the frontend
    void consume(logging::record_view const& rec);
    // The function flushes the file
    void flush();

private:
    // The function resets statistical accumulators to initial values
    void reset_accumulators();
    // The function writes the collected data to the file
    void write_data();
};
//]

// The constructor initializes the internal data
stat_collector::stat_collector(const char* file_name, boost::posix_time::time_duration write_interval) :
    m_csv_file(file_name, std::ios::app),
    m_active_connections(0),
    m_last_store_time(boost::posix_time::microsec_clock::universal_time()),
    m_write_interval(write_interval)
{
    reset_accumulators();
    if (!m_csv_file.is_open())
        throw std::runtime_error("could not open the CSV file");
}

BOOST_LOG_ATTRIBUTE_KEYWORD(sent, "Sent", unsigned int)
BOOST_LOG_ATTRIBUTE_KEYWORD(received, "Received", unsigned int)

// The function consumes the log records that come from the frontend
void stat_collector::consume(logging::record_view const& rec)
{
    // Accumulate statistical readings
    if (rec.attribute_values().count("Connected"))
        ++m_active_connections;
    else if (rec.attribute_values().count("Disconnected"))
        --m_active_connections;
    else
    {
        namespace phoenix = boost::phoenix;
        logging::visit(sent, rec, phoenix::ref(m_sent_bytes) += phoenix::placeholders::_1);
        logging::visit(received, rec, phoenix::ref(m_received_bytes) += phoenix::placeholders::_1);
    }
    ++m_collected_count;

    // Check if it's time to write the accumulated data to the file
    boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
    if (now - m_last_store_time >= m_write_interval)
    {
        write_data();
        m_last_store_time = now;
    }
}

// The function writes the collected data to the file
void stat_collector::write_data()
{
    m_csv_file << m_active_connections
        << ',' << m_sent_bytes
        << ',' << m_received_bytes
        << std::endl;
    reset_accumulators();
}

// The function resets statistical accumulators to initial values
void stat_collector::reset_accumulators()
{
    m_sent_bytes = m_received_bytes = 0;
    m_collected_count = 0;
}

// The function flushes the file
void stat_collector::flush()
{
    // Store any data that may have been collected since the list write to the file
    if (m_collected_count > 0)
    {
        write_data();
        m_last_store_time = boost::posix_time::microsec_clock::universal_time();
    }

    m_csv_file.flush();
}

//[ example_extension_stat_collector_factory
// Factory for the stat_collector sink
class stat_collector_factory :
    public logging::sink_factory< char >
{
public:
    // Creates the sink with the provided parameters
    boost::shared_ptr< sinks::sink > create_sink(settings_section const& settings)
    {
        // Read sink parameters
        std::string file_name;
        if (boost::optional< std::string > param = settings["FileName"])
            file_name = param.get();
        else
            throw std::runtime_error("No target file name specified in settings");

        boost::posix_time::time_duration write_interval = boost::posix_time::minutes(1);
        if (boost::optional< std::string > param = settings["WriteInterval"])
        {
            unsigned int sec = boost::lexical_cast< unsigned int >(param.get());
            write_interval = boost::posix_time::seconds(sec);
        }

        // Create the sink
        boost::shared_ptr< stat_collector > backend = boost::make_shared< stat_collector >(file_name.c_str(), write_interval);
        boost::shared_ptr< sinks::synchronous_sink< stat_collector > > sink = boost::make_shared< sinks::synchronous_sink< stat_collector > >(backend);

        if (boost::optional< std::string > param = settings["Filter"])
        {
            sink->set_filter(logging::parse_filter(param.get()));
        }

        return sink;
    }
};

void init_factories()
{
    logging::register_sink_factory("StatCollector", boost::make_shared< stat_collector_factory >());
}
//]

const char settings[] =
    "[Sinks.MyStat]\n"
    "Destination=StatCollector\n"
    "FileName=stat.csv\n"
    "WriteInterval=30\n"
;

void init_logging()
{
    init_factories();

    std::istringstream strm(settings);
    logging::init_from_stream(strm);
}

int main(int, char*[])
{
    init_logging();

    src::logger lg;
    BOOST_LOG(lg) << logging::add_value("Connected", true);
    BOOST_LOG(lg) << logging::add_value("Sent", 100u);
    BOOST_LOG(lg) << logging::add_value("Received", 200u);

    logging::core::get()->flush();

    return 0;
}
