#ifndef __REPLICATION_PROTOCOL_HPP__
#define __REPLICATION_PROTOCOL_HPP__

#include <boost/function.hpp>

#include "arch/arch.hpp"
#include "containers/thick_list.hpp"
#include "scoped_malloc.hpp"
#include "replication/net_structs.hpp"
#include "containers/shared_buf.hpp"

#include "data_provider.hpp"

namespace replication {

template <class T>
struct stream_pair {
    buffered_data_provider_t *stream;
    buffed_data_t<T> data;

    // This uses key_size, which is completely crap.
    stream_pair(weak_buf_t buffer, size_t beg, size_t n, size_t size = 0) : data(buffer, beg) {
        char *p;
        size_t m = sizeof(T) + buffer.get<T>(beg)->key_size;
        stream = new buffered_data_provider_t(size == 0 ? n - m : size, (void **)&p);

        memcpy(p, buffer.get<char>(beg + m), n - m);
    }

    T *operator->() { return data.get(); }
};

class message_callback_t {
public:
    // These call .swap on their parameter, taking a share of ownership of the pointee.
    virtual void hello(net_hello_t message) = 0;
    virtual void send(buffed_data_t<net_backfill_t>& message) = 0;
    virtual void send(buffed_data_t<net_announce_t>& message) = 0;
    virtual void send(buffed_data_t<net_get_cas_t>& message) = 0;
    virtual void send(stream_pair<net_sarc_t>& message) = 0;
    virtual void send(buffed_data_t<net_incr_t>& message) = 0;
    virtual void send(buffed_data_t<net_decr_t>& message) = 0;
    virtual void send(stream_pair<net_append_t>& message) = 0;
    virtual void send(stream_pair<net_prepend_t>& message) = 0;
    virtual void send(buffed_data_t<net_delete_t>& message) = 0;
    virtual void send(buffed_data_t<net_nop_t>& message) = 0;
    virtual void send(buffed_data_t<net_ack_t>& message) = 0;
    virtual void send(buffed_data_t<net_shutting_down_t>& message) = 0;
    virtual void send(buffed_data_t<net_goodbye_t>& message) = 0;
    virtual void conn_closed() = 0;
};

typedef thick_list<std::pair<boost::function<void ()>, std::pair<char *, size_t> > *, uint32_t> tracker_t;

class message_parser_t {
public:
    message_parser_t() : shutdown_asked_for(false) {}
    ~message_parser_t() {}

    void parse_messages(tcp_conn_t *conn, message_callback_t *receiver);

    struct message_parser_shutdown_callback_t {
        virtual void on_parser_shutdown() = 0;
    protected:
        ~message_parser_shutdown_callback_t() { }
    };
    bool shutdown(message_parser_shutdown_callback_t *cb);

    void co_shutdown();

private:
    size_t handle_message(message_callback_t *receiver, weak_buf_t buffer, size_t offset, size_t num_read, tracker_t& streams);
    void do_parse_messages(tcp_conn_t *conn, message_callback_t *receiver);
    void do_parse_normal_messages(tcp_conn_t *conn, message_callback_t *receiver, tracker_t& streams);

    bool keep_going; /* used to signal the parser when to stop */
    bool shutdown_asked_for; /* were we asked to shutdown (used to ignore connection exceptions */

    message_parser_shutdown_callback_t *_cb;

    DISABLE_COPYING(message_parser_t);
};
}  // namespace replication


#endif  // __REPLICATION_PROTOCOL_HPP__
