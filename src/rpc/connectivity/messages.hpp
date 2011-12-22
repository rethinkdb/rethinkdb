#ifndef __RPC_CONNECTIVITY_MESSAGES_HPP__
#define __RPC_CONNECTIVITY_MESSAGES_HPP__

#include "rpc/connectivity/connectivity.hpp"

/* `message_read_service_t` is an abstract superclass for things that let you
receive messages from other nodes. `message_write_service_t` is an abstract
superclass for things that let you send messages to other nodes.
`message_readwrite_service_t` allows both.

The reason for these abstract classes is because some of the components of the
clustering stack can be stacked on top of each other in different ways. For
example, `mailbox_manager_t` can run on top of any
`message_readwrite_service_t`; usually it's run on top of a
`directory_manager_t`, which is a `message_readwrite_service_t` and also uses a
`message_readwrite_service_t`. */

class message_read_service_t {
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
        handler_registration_t(message_read_service_t *p, const boost::function<void(peer_id_t, std::istream &)> &cb) : parent(p) {
            parent->set_message_callback(cb);
        }
        ~handler_registration_t() {
            parent->set_message_callback(NULL);
        }
    private:
        message_read_service_t *parent;
    };
    virtual connectivity_service_t *get_connectivity() = 0;
protected:
    virtual ~message_read_service_t() { }
private:
    friend class handler_registration_t;
    virtual void set_message_callback(
            const boost::function<void(
                peer_id_t source_peer,
                std::istream &stream_from_peer
                )> &callback
            ) = 0;
};

class message_write_service_t {
public:
    virtual void send_message(
            peer_id_t dest_peer,
            const boost::function<void(std::ostream &)> &writer) = 0;
    virtual connectivity_service_t *get_connectivity() = 0;
protected:
    virtual ~message_write_service_t() { }
};

class message_readwrite_service_t :
    public message_read_service_t,
    public message_write_service_t
{
public:
    virtual connectivity_service_t *get_connectivity() = 0;
};

#endif /* __RPC_CONNECTIVITY_MESSAGES_HPP__ */
