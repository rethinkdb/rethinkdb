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
    /* Whenever the message service receives a message from a peer, it calls the
    callback that is passed to the `handler_registration_t`. Its arguments are
    the peer we received the message from and a `std::istream&` from that peer.
    The callback should read the message from the stream and then handle it.
    Only one instance of the callback will be active at a time for any given
    peer, so the callback should spawn a new coroutine if it needs to perform
    any long-running computations. The callback may return without reading the
    entire message. */
    class handler_registration_t {
    public:
        handler_registration_t(message_service_t *p, const boost::function<void(peer_id_t, std::istream &)> &cb) : parent(p) {
            parent->set_message_callback(cb);
        }
        ~handler_registration_t() {
            parent->set_message_callback(NULL);
        }
    private:
        message_service_t *parent;
    };
    virtual connectivity_service_t *get_connectivity() = 0;
    virtual void send_message(
            peer_id_t dest_peer,
            const boost::function<void(std::ostream &)> &writer) = 0;
protected:
    virtual ~message_service_t() { }
private:
    friend class handler_registration_t;
    virtual void set_message_callback(
            const boost::function<void(
                peer_id_t source_peer,
                std::istream &stream_from_peer
                )> &callback
            ) = 0;
};

#endif /* __RPC_CONNECTIVITY_MESSAGES_HPP__ */
