#ifndef __REPLICATION_PROTOCOL_HPP__
#define __REPLICATION_PROTOCOL_HPP__

#include "arch/arch.hpp"
#include "containers/thick_list.hpp"
#include "scoped_malloc.hpp"
#include "replication/value_stream.hpp"
#include "replication/net_structs.hpp"

namespace replication {

template <class T>
struct stream_pair {
    value_stream_t *stream;
    scoped_malloc<T> data;
    stream_pair(const char *beg, const char *end) : stream(new value_stream_t()), data(beg, beg + sizeof(T) + reinterpret_cast<const T *>(beg)->key_size) {
        write_charslice(stream, const_charslice(beg + sizeof(T) + reinterpret_cast<const T *>(beg)->key_size, end));
    }
};

class message_callback_t {
public:
    // These call .swap on their parameter, taking ownership of the pointee.
    virtual void hello(scoped_malloc<net_hello_t>& message) = 0;
    virtual void send(scoped_malloc<net_backfill_t>& message) = 0;
    virtual void send(scoped_malloc<net_announce_t>& message) = 0;
    virtual void send(stream_pair<net_set_t>& message) = 0;
    virtual void send(stream_pair<net_append_t>& message) = 0;
    virtual void send(stream_pair<net_prepend_t>& message) = 0;
    virtual void send(scoped_malloc<net_nop_t>& message) = 0;
    virtual void send(scoped_malloc<net_ack_t>& message) = 0;
    virtual void send(scoped_malloc<net_shutting_down_t>& message) = 0;
    virtual void send(scoped_malloc<net_goodbye_t>& message) = 0;
    virtual void conn_closed() = 0;
};

class message_parser_t {
public:
    message_parser_t() : shutdown_asked_for(false) {}
    ~message_parser_t() {}

private:
    bool keep_going; /* used to signal the parser when to stop */
    bool shutdown_asked_for; /* were we asked to shutdown (used to ignore connection exceptions */

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
    void handle_message(message_callback_t *receiver, const char *msgbegin, const char *msgend, thick_list<value_stream_t *, uint32_t>& streams);
    void do_parse_messages(tcp_conn_t *conn, message_callback_t *receiver);
    void do_parse_normal_message(tcp_conn_t *conn, message_callback_t *receiver, thick_list<value_stream_t *, uint32_t>& streams);

};
}  // namespace replication


#endif  // __REPLICATION_PROTOCOL_HPP__
