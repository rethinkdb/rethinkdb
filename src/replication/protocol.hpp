#ifndef __REPLICATION_PROTOCOL_HPP__
#define __REPLICATION_PROTOCOL_HPP__

#include <boost/scoped_ptr.hpp>

#include "arch/arch.hpp"
#include "replication/messages.hpp"

namespace replication {

class message_callback_t {
public:
    // These call .swap on their parameter, taking ownership of the pointee.
    virtual void hello(boost::scoped_ptr<hello_message_t>& message) = 0;
    virtual void send(boost::scoped_ptr<backfill_message_t>& message) = 0;
    virtual void send(boost::scoped_ptr<announce_message_t>& message) = 0;
    virtual void send(boost::scoped_ptr<set_message_t>& message) = 0;
    virtual void send(boost::scoped_ptr<append_message_t>& message) = 0;
    virtual void send(boost::scoped_ptr<prepend_message_t>& message) = 0;
    virtual void send(boost::scoped_ptr<nop_message_t>& message) = 0;
    virtual void send(boost::scoped_ptr<ack_message_t>& message) = 0;
    virtual void send(boost::scoped_ptr<shutting_down_message_t>& message) = 0;
    virtual void send(boost::scoped_ptr<goodbye_message_t>& message) = 0;
    virtual void conn_closed() = 0;
};

class message_parser_t {
public:
    message_parser_t() {}
    ~message_parser_t() {}

private:
    bool keep_going; /* used to signal the parser when to stop */

public:
    void parse_messages(tcp_conn_t *conn, message_callback_t *receiver);
    void stop_parsing();

private:
    void do_parse_messages(tcp_conn_t *conn, message_callback_t *receiver);
    void do_parse_normal_message(tcp_conn_t *conn, message_callback_t *receiver, std::vector<value_stream_t *>& streams);

    template <class message_type>
        bool parse_and_stream(tcp_conn_t *conn, message_callback_t *receiver, tcp_conn_t::bufslice sl, std::vector<value_stream_t *>& streams);

    template <class message_type>
        bool parse_and_pop(tcp_conn_t *conn, message_callback_t *receiver, const char *buffer, size_t size);

    template <class message_type>
        ssize_t try_parsing(message_callback_t *receiver, const char *buffer, size_t size);

    template <class struct_type>
        ssize_t objsize(const struct_type *buffer);

    template <class struct_type>
        bool fits(const char *buffer, size_t size);

    void do_parse_hello_message(tcp_conn_t *conn, message_callback_t *receiver);

    bool valid_role(uint32_t val);
};
}  // namespace replication


#endif  // __REPLICATION_PROTOCOL_HPP__
