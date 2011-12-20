#ifndef __RPC_CONNECTIVITY_MESSAGES_HPP__
#define __RPC_CONNECTIVITY_MESSAGES_HPP__

#include "rpc/connectivity/connectivity.hpp"

/* `message_service_t` is an abstract superclass for things that allow you to
send messages directly to other nodes and receive messages from other nodes. The
reason why we need such an abstract class is that some of the clustering stack
components build on each other. Specifically, `connectivity_cluster_t` exposes a
`message_service_t` to `directory_manager_t`, which in turn exposes a
`message_service_t` to `mailbox_manager_t`. */

class message_service_t {
public:
    virtual connectivity_service_t *get_connectivity() = 0;
    virtual void send_message(
            peer_id_t dest_peer,
            const boost::function<void(std::ostream &)> &writer) = 0;
    virtual void set_message_callback(
            const boost::function<void(
                peer_id_t source_peer,
                std::istream &stream_from_peer,
                const boost::function<void()> &call_when_done
                )> &callback
            ) = 0;
protected:
    virtual ~message_service_t() { }
};

#endif /* __RPC_CONNECTIVITY_MESSAGES_HPP__ */
