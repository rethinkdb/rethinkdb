#ifndef __RPC_DIRECTORY_DIRECTORY_HPP__
#define __RPC_DIRECTORY_DIRECTORY_HPP__

#include "rpc/mailbox/mailbox.hpp"

template<class metadata_t>
class directory_manager_t :
    public connectivity_service_t,
    public directory_service_t,
    
    public home_thread_mixin_t
{
public:
    directory_manager_t(message_service_t *ms, const metadata_t &initial_metadata) THROWS_NOTHING;

    boost::shared_ptr<directory_rwview_t<metadata_t> > get_root_view() THROWS_NOTHING;

    /* `connectivity_service_t` interface */
    peer_id_t get_me();
    std::set<peer_id_t> get_peers_list();

    /* `directory_service_t` interface */
    connectivity_service_t *get_connectivity();

    /* Returns a `message_service_t` that can be used to send non-directory
    communications between peers. The returned message service's
    `get_connectivity()` will return `this`. */
    message_service_t *get_sub_message_service();

private:
    /* `get_root_view()` returns a pointer to this. */
    class root_view_t : public directory_rwview_t<metadata_t> {
    public:
        boost::optional<metadata_t> get_value(peer_id_t peer) THROWS_NOTHING;
        directory_t *get_directory() THROWS_NOTHING;
        peer_id_t get_me() THROWS_NOTHING;
        void set_our_value(const metadata_t &new_value_for_us, directory_t::our_value_lock_acq_t *proof) THROWS_NOTHING;
    private:
        friend class directory_cluster_t;
        explicit root_view_t(directory_cluster_t *) THROWS_NOTHING;
        directory_cluster_t *parent;
    };

    class sub_message_service_t : public message_service_t {
    public:
        sub_message_service_t(directory_manager_t *p);
        connectivity_service_t *get_connectivity();
        void send_message(
                peer_id_t dest_peer,
                const boost::function<void(std::ostream &)> &writer);
        void set_message_callback(
                const boost::function<void(
                    peer_id_t source_peer,
                    std::istream &stream_from_peer,
                    const boost::function<void()> &call_when_done
                    )> &callback
                );
    private:
        directory_manager_t *parent;
    };

    class peer_info_t {
    public:
        peer_info_t(const metadata_t &md) : value(md) { }
        mutex_assertion_t peer_value_lock;
        metadata_t value;
        publisher_controller_t<
                boost::function<void()>
                > peer_value_publisher;
    };

    class thread_info_t {
    public:
        thread_info_t(peer_id_t me, const metadata_t &md) {
            peers.insert(me, new peer_info_t(md));
        }
        boost::ptr_map<peer_id_t, peer_info_t> peers;
    };

    void on_message(peer_id_t source, std::istream &stream, const boost::function<void()> &on_done) THROWS_NOTHING;
    void propagate_to_peer(peer_id_t peer, const metadata_t &value) THROWS_NOTHING;
    void propagate_on_thread(int thread, peer_id_t peer, const metadata_t &value) THROWS_NOTHING;

    /* `directory_service_t` methods */
    mutex_assertion_t *get_peer_value_lock(peer_id_t) THROWS_NOTHING;
    publisher_t<
            boost::function<void()>
            > *get_peer_value_publisher(peer_id_t, peer_value_freeze_t *proof) THROWS_NOTHING;
    mutex_t *get_our_value_lock() THROWS_NOTHING;
    int get_our_value_lock_thread() THROWS_NOTHING;

    /* The message service that we use to communicate with other peers */
    message_service_t *super_message_service;

    /* The message service that we expose to other components, whose
    connectivity service is us */
    sub_message_service_t sub_message_service;

    boost::shared_ptr<root_view_t> root_view;

    /* `our_value` is our current metadata value. It's usually equal to
    `thread_info[...]->peers[super_message_service->get_me()]->value`. The
    difference is that `our_value` is used for propagation to other machines
    whereas `thread_info[...]->...->value` is used for propagation to other
    threads on the same machine, so they are locked differently. */
    metadata_t our_value;

    /* Incremented every time `our_value` is changed. */
    state_timestamp_t our_value_timestamp;

    one_per_thread_t<thread_info_t> thread_info;

    /* Held when `our_value` is being changed. */
    mutex_t our_value_lock;

    /* `auto_drainer` is used to make sure that all propagation operations are
    completed before we start calling the destructors on our internal variables.
    */
    auto_drainer_t auto_drainer;
};

#include "rpc/directory/directory.tcc"

#endif /* __RPC_DIRECTORY_DIRECTORY_HPP__ */
