#ifndef __REPLICATION_SLAVE_HPP__
#define __REPLICATION_SLAVE_HPP__

#include <boost/scoped_ptr.hpp>

#undef assert
#include "arch/arch.hpp"

namespace replication {

class hello_message_t;
class backfill_message_t;
class announce_message_t;
class set_message_t;
class append_message_t;
class prepend_message_t;
class nop_message_t;
class ack_message_t;
class shutting_down_message_t;
class goodbye_message_t;

class message_callback_t {
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

class parser_t : public linux_net_conn_read_external_callback_t {
public:
    parser_t(net_conn_t *conn, message_callback_t *receiver);
    ~parser_t();

    void on_net_conn_read_external();
    void on_net_conn_read_buffered(const char *buffer, size_t size);
    void on_net_conn_close();

    void handle_hello_message(const char *buffer, size_t size);
    void handle_nonhello_message(const char *buffer, size_t size);


private:
    net_conn_t * const conn_;
    message_callback_t * const receiver_;

    bool hello_message_received_;
    static const int PARSER_BUFFER_SIZE = 1024;
    char buf_[PARSER_BUFFER_SIZE];

    void *fillee_;

    DISABLE_COPYING(parser_t);
};



}  // namespace replication




#endif  // __REPLICATION_SLAVE_HPP__
