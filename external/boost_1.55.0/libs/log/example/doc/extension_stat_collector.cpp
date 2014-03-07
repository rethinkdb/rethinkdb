/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include <string>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/phoenix.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/basic_sink_backend.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/attributes/value_visitation.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;

//[ example_extension_stat_collector_definition
// The backend collects statistical information about network activity of the application
class stat_collector :
    public sinks::basic_sink_backend<
        sinks::combine_requirements<
            sinks::synchronized_feeding,                                        /*< we will have to store internal data, so let's require frontend to synchronize feeding calls to the backend >*/
            sinks::flushing                                                     /*< also enable flushing support >*/
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

public:
    // The constructor initializes the internal data
    explicit stat_collector(const char* file_name);

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
stat_collector::stat_collector(const char* file_name) :
    m_csv_file(file_name, std::ios::app),
    m_active_connections(0),
    m_last_store_time(boost::posix_time::microsec_clock::universal_time())
{
    reset_accumulators();
    if (!m_csv_file.is_open())
        throw std::runtime_error("could not open the CSV file");
}

//[ example_extension_stat_collector_consume
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
    if (now - m_last_store_time >= boost::posix_time::minutes(1))
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
//]

//[ example_extension_stat_collector_flush
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
//]

// Complete sink type
typedef sinks::synchronous_sink< stat_collector > sink_t;

void init_logging()
{
    boost::shared_ptr< logging::core > core = logging::core::get();

    boost::shared_ptr< stat_collector > backend(new stat_collector("stat.csv"));
    boost::shared_ptr< sink_t > sink(new sink_t(backend));
    core->add_sink(sink);
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
