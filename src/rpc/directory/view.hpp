#ifndef __RPC_DIRECTORY_VIEW_HPP__
#define __RPC_DIRECTORY_VIEW_HPP__

#include "concurrency/pubsub.hpp"
#include "rpc/connectivity/connectivity.hpp"

class directory_t : public home_thread_mixin_t {
public:
    class peers_list_freeze_t {
    public:
        peers_list_freeze_t(directory_t *) THROWS_NOTHING;
        void assert_is_holding(directory_t *) THROWS_NOTHING;
    private:
        directory_t *directory;
        mutex_assertion_t::acq_t acq;
    };

    class peers_list_subscription_t {
    public:
        peers_list_subscription_t(
            const boost::function<void(peer_id_t)> &connect_cb,
            const boost::function<void(peer_id_t)> &disconnect_cb) THROWS_NOTHING;
        peers_list_subscription_t(
            const boost::function<void(peer_id_t)> &connect_cb,
            const boost::function<void(peer_id_t)> &disconnect_cb,
            directory_t *, peers_list_freeze_t *proof) THROWS_NOTHING;
        void reset() THROWS_NOTHING;
        void reset(directory_t *, peers_list_freeze_t *proof) THROWS_NOTHING;
    private:
        publisher_t<std::pair<
            boost::function<void(peer_id_t)>,
            boost::function<void(peer_id_t)>
            > >::subscription_t subs;
    };

    class peer_value_freeze_t {
    public:
        peer_value_freeze_t(directory_t *, peer_id_t) THROWS_NOTHING;
        void assert_is_holding(directory_t *, peer_id_t) THROWS_NOTHING;
    private:
        directory_t *directory;
        peer_id_t peer;
        mutex_assertion_t::acq_t acq;
    };

    class peer_value_subscription_t {
    public:
        peer_value_subscription_t(
            const boost::function<void()> &change_cb) THROWS_NOTHING;
        peer_value_subscription_t(
            const boost::function<void()> &change_cb,
            directory_t *, peer_id_t, peer_value_freeze_t *proof) THROWS_NOTHING;
        void reset() THROWS_NOTHING;
        void reset(directory_t *, peer_id_t, peer_value_freeze_t *proof) THROWS_NOTHING;
    private:
        publisher_t<
            boost::function<void()>
            >::subscription_t subs;
    };

    class our_value_lock_acq_t {
    public:
        our_value_lock_acq_t(directory_t *) THROWS_NOTHING;
        void assert_is_holding(directory_t *);
    private:
        directory_t *directory;
        mutex_t::acq_t acq;
    };

    virtual std::set<peer_id_t> get_peer_list() THROWS_NOTHING = 0;

protected:
    virtual ~directory_t() THROWS_NOTHING { }

private:
    friend class peers_list_freeze_t;
    friend class peers_list_subscription_t;
    friend class peer_value_freeze_t;
    friend class peer_value_subscription_t;
    virtual mutex_assertion_t *get_peers_list_lock() THROWS_NOTHING = 0;
    virtual publisher_t<std::pair<
        boost::function<void(peer_id_t)>,
        boost::function<void(peer_id_t)>
        > > *get_peer_list_publisher(peers_list_lock_acq_t *proof) THROWS_NOTHING = 0;
    virtual mutex_assertion_t *get_peer_value_lock(peer_id_t) THROWS_NOTHING = 0;
    virtual publisher_t<
        boost::function<void()>
        > *get_peer_value_publisher(peer_id_t, peer_value_lock_acq_t *proof) THROWS_NOTHING = 0;
    virtual mutex_t *get_our_value_lock() THROWS_NOTHING = 0;
};

template<class metadata_t>
class directory_rview_t {
public:
    virtual boost::optional<metadata_t> get_value(peer_id_t peer) THROWS_NOTHING = 0;
    virtual directory_t *get_directory() THROWS_NOTHING = 0;
protected:
    virtual ~directory_rview_t() THROWS_NOTHING { }
};

template<class metadata_t>
class directory_rwview_t : public directory_rview_t<metadata_t> {
public:
    virtual peer_id_t get_me() THROWS_NOTHING = 0;
    virtual void set_our_value(const metadata_t &new_value_for_us, directory_t::our_value_lock_acq_t *proof) THROWS_NOTHING = 0;
};

#endif /* __RPC_DIRECTORY_VIEW_HPP__ */
