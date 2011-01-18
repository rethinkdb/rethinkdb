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

void parse_messages(tcp_conn_t *conn, message_callback_t *receiver);


}  // namespace replication




#endif  // __REPLICATION_SLAVE_HPP__
