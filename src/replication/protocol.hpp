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

public:
    struct message_parser_shutdown_callback_t {
        virtual void on_parser_shutdown() = 0;
    };
    bool shutdown(message_parser_shutdown_callback_t *cb);

private:
    message_parser_shutdown_callback_t *_cb;

private:
    void do_parse_messages(tcp_conn_t *conn, message_callback_t *receiver);
    void do_parse_normal_message(tcp_conn_t *conn, message_callback_t *receiver, std::vector<value_stream_t *>& streams);

};
}  // namespace replication


#endif  // __REPLICATION_PROTOCOL_HPP__
