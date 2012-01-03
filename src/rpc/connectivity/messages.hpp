#ifndef __RPC_CONNECTIVITY_MESSAGES_HPP__
#define __RPC_CONNECTIVITY_MESSAGES_HPP__

#include "rpc/connectivity/connectivity.hpp"

/* `message_service_t` is an abstract superclass for things that let you send
messages to other nodes. `message_handler_t` is an abstract superclass for
things that handle messages received from other nodes. The general pattern
usually looks something like this:

    class cluster_t : public message_service_t {
    public:
        class run_t {
        public:
            run_t(cluster_t *, message_handler_t *);
        };
        ...
    };

    class application_t : public message_handler_t {
    public:
        application_t(message_service_t *);
        ...
    };

    void do_cluster() {
        cluster_t cluster;
        application_t app(&cluster);
        cluster_t::run_t cluster_run(&cluster, &app);
        ...
    }

The rationale for splitting the lower-level messaging stuff into two components
(e.g. `cluster_t` and `cluster_t::run_t`) is that it makes it clear how to stop
further messages from being delivered. When the `cluster_t::run_t` is destroyed,
no further messages are delivered. This gives a natural way to make sure that no
messages are still being delivered at the time that the `application_t`
destructor is called. */

class message_service_t  {
public:
    virtual void send_message(
            peer_id_t dest_peer,
            const boost::function<void(std::ostream &)> &writer
            ) = 0;
    virtual connectivity_service_t *get_connectivity_service() = 0;
protected:
    virtual ~message_service_t() { }
};

class message_handler_t {
public:
    virtual void on_message(
            peer_id_t source_peer,
            std::istream &stream_from_peer
            ) = 0;
protected:
    virtual ~message_handler_t() { }
};

#endif /* __RPC_CONNECTIVITY_MESSAGES_HPP__ */
