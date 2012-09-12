#ifndef RPC_CONNECTIVITY_MESSAGES_HPP_
#define RPC_CONNECTIVITY_MESSAGES_HPP_

class connectivity_service_t;
class peer_id_t;
class read_stream_t;
class write_stream_t;

namespace boost {
template <class> class function;
}

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

class send_message_write_callback_t {
public:
    virtual ~send_message_write_callback_t() { }
    virtual void write(write_stream_t *stream) = 0;
};

class message_service_t  {
public:
    virtual void send_message(peer_id_t dest_peer, send_message_write_callback_t *callback) = 0;
    virtual connectivity_service_t *get_connectivity_service() = 0;
protected:
    virtual ~message_service_t() { }
};

class message_handler_t {
public:
    virtual void on_message(peer_id_t source_peer, read_stream_t *) = 0;
protected:
    virtual ~message_handler_t() { }
};

#endif /* RPC_CONNECTIVITY_MESSAGES_HPP_ */
