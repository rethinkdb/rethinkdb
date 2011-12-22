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
    ~directory_manager_t() THROWS_NOTHING;

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
        friend class directory_manager_t;
        directory_manager_t *parent;
        boost::function<void(
                peer_id_t source_peer,
                std::istream &stream_from_peer,
                const boost::function<void()> &call_when_done
                )> callback;
    };

    class thread_peer_info_t {
    public:
        thread_peer_info_t(const metadata_t &md, fifo_enforcer_source_t::state_t st) :
            value(md), change_fifo_sink(st) { }
        mutex_assertion_t peer_value_lock;
        metadata_t value;
        fifo_enforcer_sink_t change_fifo_sink;
        publisher_controller_t<
                boost::function<void()>
                > peer_value_publisher;
    };

    class thread_info_t {
    public:
        fifo_enforcer_sink_t connectivity_fifo_sink;
        mutex_assertion_t peers_list_lock;
        publisher_controller_t<std::pair<
                boost::function<void(peer_id_t)>,
                boost::function<void(peer_id_t)>
                > > peers_list_publisher;
        boost::ptr_map<peer_id_t, peer_info_t> peers;
    };

    class global_peer_info_t {
    public:
        auto_drainer_t drainer;
        cond_t got_initial_message;
    };

    static void write_initial_message(std::ostream &stream, const metadata_t &initial_value, fifo_enforcer_source_t::state_t state) THROWS_NOTHING;
    static void write_update_message(std::ostream &stream, const metadata_t &new_value, fifo_enforcer_write_token_t fifo_token) THROWS_NOTHING;
    static void write_sub_message(std::ostream &stream, const boost::function<void(std::ostream &)> &sub_function) THROWS_NOTHING;

    void on_connect(peer_id_t peer) THROWS_NOTHING;
    void on_message(peer_id_t source, std::istream &stream) THROWS_NOTHING;
    void on_disconnect(peer_id_t peer) THROWS_NOTHING;

    void greet_peer(auto_drainer_t::lock_t global_keepalive, peer_id_t peer, const metadata_t &initial_value, fifo_enforcer_source_t::state_t state) THROWS_NOTHING;
    void propagate_initial_on_thread(int dest_thread, fifo_enforcer_write_token_t connectivity_fifo_token, auto_drainer_t::lock_t global_keepalive, peer_id_t peer, const metadata_t &initial_value, fifo_enforcer_source_t::state_t fifo_state) THROWS_NOTHING;
    void propagate_update_everywhere(auto_drainer_t::lock_t global_keepalive, peer_id_t peer, const metadata_t &new_value, fifo_enforcer_write_token_t change_fifo_token, auto_drainer_t::lock_t peer_keepalive) THROWS_NOTHING;
    void propagate_update_on_thread(int dest_thread, fifo_enforcer_read_token_t connectivity_fifo_token, auto_drainer_t::lock_t global_keepalive, peer_id_t peer, const metadata_t &new_value, fifo_enforcer_write_token_t change_fifo_token, auto_drainer_t::lock_t peer_keepalive) THROWS_NOTHING;
    void drain_propagate_update(auto_drainer_t::lock_t global_keepalive, auto_drainer_t::lock_t *destroy_me) THROWS_NOTHING;
    void propagate_disconnect_on_thread(int dest_thread, fifo_enforcer_write_token_t connectivity_fifo_token, auto_drainer_t::lock_t global_keepalive, peer_id_t peer) THROWS_NOTHING;

    /* `connectivity_service_t` methods */
    mutex_assertion_t *get_peers_list_lock();
    publisher_t<std::pair<
            boost::function<void(peer_id_t)>,
            boost::function<void(peer_id_t)>
            > > *get_peers_list_publisher();

    /* `directory_service_t` methods */
    mutex_assertion_t *get_peer_value_lock(peer_id_t) THROWS_NOTHING;
    publisher_t<
            boost::function<void()>
            > *get_peer_value_publisher(peer_id_t, peer_value_freeze_t *proof) THROWS_NOTHING;
    mutex_t *get_our_value_lock() THROWS_NOTHING;

    /* The message service that we use to communicate with other peers */
    message_service_t *super_message_service;

    /* The message service that we expose to other components, whose
    connectivity service is us */
    sub_message_service_t sub_message_service;

    boost::shared_ptr<root_view_t> root_view;

    /* This is the lock that gets held when someone constructs an instance of
    `directory_service_t::our_value_lock_acq_t`. */
    mutex_t our_value_lock;

    /* This lock is held for brief periods during actual calls to
    `set_our_value()` or when we are sending our value to new peers. It protects
    `our_value` and `change_fifo_source`. */
    mutex_assertion_t our_value_lock_assertion;

    /* `our_value` is our current metadata value. It's usually equal to
    `thread_info[...]->peers[super_message_service->get_me()]->value`. The
    difference is that `our_value` is used for propagation to other machines
    whereas `thread_info[...]->...->value` is used for propagation to other
    threads on the same machine, so they are locked differently. `our_value`
    should be accessed only from `home_thread()`. */
    metadata_t our_value;

    /* Used to ensure that if we make two changes to our metadata in quick
    succession, they will be performed in the correct order even if they get
    reordered en route to their destination. Should only be accessed from
    `home_thread()`. */
    fifo_enforcer_source_t change_fifo_source;

    one_per_thread_t<thread_info_t> thread_info;

    /* This lock protects `connectivity_fifo_source` and `peer_drainers`. */
    mutex_assertion_t connectivity_lock_assertion;

    /* Used to make sure that notifications of peers connecting/disconnecting or
    changing their values are not reordered en route to the other threads of
    this machine. */
    fifo_enforcer_source_t connectivity_fifo_source;

    /* Used to notify `propagate_update()` when the peer connection has been
    lost. */
    boost::ptr_map<peer_id_t, global_peer_info_t> peer_info;

    auto_drainer_t drainer;

    connectivity_service_t::peers_list_subscription_t connectivity_subscription;
    message_service_t::handler_registration_t message_handler_registration;
};

#include "rpc/directory/directory.tcc"

#endif /* __RPC_DIRECTORY_DIRECTORY_HPP__ */
