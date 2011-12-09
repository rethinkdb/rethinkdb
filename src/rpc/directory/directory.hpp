#ifndef __RPC_DIRECTORY_DIRECTORY_HPP__
#define __RPC_DIRECTORY_DIRECTORY_HPP__

#include "rpc/mailbox/mailbox.hpp"

template<class metadata_t>
class directory_cluster_t :
    public directory_t,
    private event_watcher_t
{
public:
    directory_cluster_t(int port, const metadata_t &initial_metadata) THROWS_NOTHING;
    ~directory_cluster_t() THROWS_NOTHING;

    boost::shared_ptr<directory_rwview_t<metadata_t> > get_root_view() THROWS_NOTHING;

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

    class peer_info_t {
        mutex_assertion_t peer_value_lock;
        metadata_t value;
        publisher_controller_t<
                boost::function<void()>
                > peer_value_publisher;
    };

    class thread_info_t {
        mutex_assertion_t peers_list_lock;
        boost::ptr_map<peer_id_t, peer_info_t> peers;
        publisher_controller_t<std::pair<
                boost::function<void(peer_id_t)>,
                boost::function<void(peer_id_t)>
                > > peers_list_publisher;
    };

    void construct_on_thread(int thread, const metadata_t &initial_metadata) THROWS_NOTHING;
    void destruct_on_thread(int thread) THROWS_NOTHING;
    void propagate_on_thread(int thread, peer_id_t peer, const metadata_t &value) THROWS_NOTHING;

    /* `directory_t` methods */
    mutex_assertion_t *get_peers_list_lock() THROWS_NOTHING;
    publisher_t<std::pair<
            boost::function<void(peer_id_t)>,
            boost::function<void(peer_id_t)>
            > > *get_peer_list_publisher(peers_list_freeze_t *proof) THROWS_NOTHING;
    mutex_assertion_t *get_peer_value_lock(peer_id_t) THROWS_NOTHING;
    publisher_t<
            boost::function<void()>
            > *get_peer_value_publisher(peer_id_t, peer_value_freeze_t *proof) THROWS_NOTHING;
    mutex_t *get_our_value_lock() THROWS_NOTHING;

    boost::shared_ptr<root_view_t> root_view;

    boost::scoped_array<boost::scoped_ptr<thread_info_t> > threads;

    mutex_t our_value_lock;

    boost::scoped_ptr<auto_drainer_t> auto_drainer;
};

#endif /* __RPC_DIRECTORY_DIRECTORY_HPP__ */
