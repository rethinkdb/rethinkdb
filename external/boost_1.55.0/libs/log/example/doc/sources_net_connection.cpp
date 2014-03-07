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
#include <boost/move/utility.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes/constant.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>
#include <boost/log/attributes/attribute_value_impl.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace attrs = boost::log::attributes;

BOOST_LOG_ATTRIBUTE_KEYWORD(line_id, "LineID", unsigned int)
BOOST_LOG_ATTRIBUTE_KEYWORD(remote_address, "RemoteAddress", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(received_size, "ReceivedSize", std::size_t)
BOOST_LOG_ATTRIBUTE_KEYWORD(sent_size, "SentSize", std::size_t)

//[ example_sources_network_connection
class network_connection
{
    src::logger m_logger;
    logging::attribute_set::iterator m_remote_addr;

public:
    void on_connected(std::string const& remote_addr)
    {
        // Put the remote address into the logger to automatically attach it
        // to every log record written through the logger
        m_remote_addr = m_logger.add_attribute("RemoteAddress",
            attrs::constant< std::string >(remote_addr)).first;

        // The straightforward way of logging
        if (logging::record rec = m_logger.open_record())
        {
            rec.attribute_values().insert("Message",
                attrs::make_attribute_value(std::string("Connection established")));
            m_logger.push_record(boost::move(rec));
        }
    }
    void on_disconnected()
    {
        // The simpler way of logging: the above "if" condition is wrapped into a neat macro
        BOOST_LOG(m_logger) << "Connection shut down";

        // Remove the attribute with the remote address
        m_logger.remove_attribute(m_remote_addr);
    }
    void on_data_received(std::size_t size)
    {
        // Put the size as an additional attribute
        // so it can be collected and accumulated later if needed.
        // The attribute will be attached only to this log record.
        BOOST_LOG(m_logger) << logging::add_value("ReceivedSize", size) << "Some data received";
    }
    void on_data_sent(std::size_t size)
    {
        BOOST_LOG(m_logger) << logging::add_value("SentSize", size) << "Some data sent";
    }
};
//]

int main(int, char*[])
{
    // Construct the sink
    typedef sinks::synchronous_sink< sinks::text_ostream_backend > text_sink;
    boost::shared_ptr< text_sink > sink = boost::make_shared< text_sink >();

    // Add a stream to write log to
    sink->locked_backend()->add_stream(
        boost::make_shared< std::ofstream >("sample.log"));

    // Set the formatter
    sink->set_formatter
    (
        expr::stream
            << line_id
            << ": [" << remote_address << "] "
            << expr::if_(expr::has_attr(received_size))
               [
                    expr::stream << "[Received: " << received_size << "] "
               ]
            << expr::if_(expr::has_attr(sent_size))
               [
                    expr::stream << "[Sent: " << sent_size << "] "
               ]
            << expr::smessage
    );

    // Register the sink in the logging core
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
