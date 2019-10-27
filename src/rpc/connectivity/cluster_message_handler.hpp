#ifndef RPC_CONNECTIVITY_CLUSTER_MESSAGE_HANDLER_HPP_
#define RPC_CONNECTIVITY_CLUSTER_MESSAGE_HANDLER_HPP_

class connectivity_cluster_t;
using connectivity_cluster_message_tag_t = uint8_t;

/* Subclass `cluster_message_handler_t` to handle messages received over the network. The
`cluster_message_handler_t` constructor will automatically register it to handle
messages. You can only register and unregister message handlers when there is no `run_t`
in existence. */
class cluster_message_handler_t {
public:
    connectivity_cluster_t *get_connectivity_cluster() {
        return connectivity_cluster;
    }
    connectivity_cluster_message_tag_t get_message_tag() const { return tag; }

    peer_id_t get_me() {
        return connectivity_cluster->get_me();
    }

protected:
    /* Registers the message handler with the cluster */
    cluster_message_handler_t(connectivity_cluster_t *connectivity_cluster,
                              connectivity_cluster_message_tag_t tag);
    virtual ~cluster_message_handler_t();

    /* This can be called on any thread. */
    virtual void on_message(connectivity_cluster_t::connection_t *conn,
                            auto_drainer_t::lock_t keepalive,
                            read_stream_t *) = 0;

    /* The default implementation constructs a stream reading from `data` and then
    calls `on_message()`. Override to optimize for the local case. */
    virtual void on_local_message(connectivity_cluster_t::connection_t *conn,
                                  auto_drainer_t::lock_t keepalive,
                                  std::vector<char> &&data);

private:
    friend class connectivity_cluster_t;
    connectivity_cluster_t *connectivity_cluster;
    const connectivity_cluster_message_tag_t tag;
};


#endif // RPC_CONNECTIVITY_CLUSTER_MESSAGE_HANDLER_HPP_
