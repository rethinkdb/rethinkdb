/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include <cstddef>
#include <string>
#include <fstream>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes/constant.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>
#include <boost/log/sources/channel_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace attrs = boost::log::attributes;
namespace keywords = boost::log::keywords;

BOOST_LOG_ATTRIBUTE_KEYWORD(line_id, "LineID", unsigned int)
BOOST_LOG_ATTRIBUTE_KEYWORD(channel, "Channel", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(remote_address, "RemoteAddress", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(received_size, "ReceivedSize", std::size_t)
BOOST_LOG_ATTRIBUTE_KEYWORD(sent_size, "SentSize", std::size_t)

//[ example_sources_network_connection_dynamic_channels
// Define a global logger
BOOST_LOG_INLINE_GLOBAL_LOGGER_CTOR_ARGS(my_logger, src::channel_logger_mt< >, (keywords::channel = "general"))

class network_connection
{
    std::string m_remote_addr;

public:
    void on_connected(std::string const& remote_addr)
    {
        m_remote_addr = remote_addr;

        // Put message to the "net" channel
        BOOST_LOG_CHANNEL(my_logger::get(), "net")
            << logging::add_value("RemoteAddress", m_remote_addr)
            << "Connection established";
    }

    void on_disconnected()
    {
        // Put message to the "net" channel
        BOOST_LOG_CHANNEL(my_logger::get(), "net")
            << logging::add_value("RemoteAddress", m_remote_addr)
            << "Connection shut down";

        m_remote_addr.clear();
    }

    void on_data_received(std::size_t size)
    {
        BOOST_LOG_CHANNEL(my_logger::get(), "stat")
            << logging::add_value("RemoteAddress", m_remote_addr)
            << logging::add_value("ReceivedSize", size)
            << "Some data received";
    }

    void on_data_sent(std::size_t size)
    {
        BOOST_LOG_CHANNEL(my_logger::get(), "stat")
            << logging::add_value("RemoteAddress", m_remote_addr)
            << logging::add_value("SentSize", size)
            << "Some data sent";
    }
};
//]

int main(int, char*[])
{
    // Construct the sink for the "net" channel
    typedef sinks::synchronous_sink< sinks::text_ostream_backend > text_sink;
    boost::shared_ptr< text_sink > sink = boost::make_shared< text_sink >();

    sink->locked_backend()->add_stream(
        boost::make_shared< std::ofstream >("net.log"));

    sink->set_formatter
    (
        expr::stream << line_id << ": [" << remote_address << "] " << expr::smessage
    );

    sink->set_filter(channel == "net");

    logging::core::get()->add_sink(sink);

    // Construct the sink for the "stat" channel
    sink = boost::make_shared< text_sink >();

    sink->locked_backend()->add_stream(
        boost::make_shared< std::ofstream >("stat.log"));

    sink->set_formatter
    (
        expr::stream
            << remote_address
            << expr::if_(expr::has_attr(received_size))
               [
                    expr::stream << " -> " << received_size << " bytes: "
               ]
            << expr::if_(expr::has_attr(sent_size))
               [
                    expr::stream << " <- " << sent_size << " bytes: "
               ]
            << expr::smessage
    );

    sink->set_filter(channel == "stat");

    logging::core::get()->add_sink(sink);

    // Register other common attributes, such as time stamp and record counter
    logging::add_common_attributes();

    // Emulate network activity
    network_connection conn;

    conn.on_connected("11.22.33.44");
    conn.on_data_received(123);
    conn.on_data_sent(321);
    conn.on_disconnected();

    return 0;
}
