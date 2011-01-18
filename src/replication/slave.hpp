#ifndef __REPLICATION_SLAVE_HPP__
#define __REPLICATION_SLAVE_HPP__

#include <boost/scoped_ptr.hpp>
#undef assert

#include "arch/arch.hpp"
#include "replication/messages.hpp"

namespace replication {

class message_callback_t {
public:
    // These call .swap on their parameter, taking ownership of the pointee.
    void hello(boost::scoped_ptr<hello_message_t>& message);
    void send(boost::scoped_ptr<backfill_message_t>& message);
    void send(boost::scoped_ptr<announce_message_t>& message);
    void send(boost::scoped_ptr<set_message_t>& message);
    void send(boost::scoped_ptr<append_message_t>& message);
    void send(boost::scoped_ptr<prepend_message_t>& message);
    void send(boost::scoped_ptr<nop_message_t>& message);
    void send(boost::scoped_ptr<ack_message_t>& message);
    void send(boost::scoped_ptr<shutting_down_message_t>& message);
    void send(boost::scoped_ptr<goodbye_message_t>& message);
    void conn_closed();
};

class parser_t : public linux_net_conn_read_external_callback_t, public linux_net_conn_read_buffered_callback_t {
public:
    parser_t(net_conn_t *conn, message_callback_t *receiver);

    void on_net_conn_read_external();
    void on_net_conn_read_buffered(const char *buffer, size_t size);
    void on_net_conn_close();

    void report_protocol_error(const char *msg);

private:
    void ask_for_a_message();

    template <class struct_type>
    void try_parsing(const char *buffer, size_t size);

    void handle_hello_message(const char *buffer, size_t size);
    void handle_nonhello_message(const char *buffer, size_t size);


    net_conn_t * const conn_;
    message_callback_t * const receiver_;

    bool hello_message_received_;
    static const int PARSER_BUFFER_SIZE = 1024;
    char buf_[PARSER_BUFFER_SIZE];

    void *fillee_;

    static const char STANDARD_HELLO_MAGIC[16];
    static const uint32_t ACCEPTED_PROTOCOL_VERSION = 1;

    DISABLE_COPYING(parser_t);
};



}  // namespace replication




#endif  // __REPLICATION_SLAVE_HPP__
