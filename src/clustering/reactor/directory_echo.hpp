#ifndef CLUSTERING_REACTOR_DIRECTORY_ECHO_HPP_
#define CLUSTERING_REACTOR_DIRECTORY_ECHO_HPP_

#include "utils.hpp"
#include <boost/optional.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/serialization/optional.hpp>

#include "rpc/directory/view.hpp"
#include "rpc/mailbox/typed.hpp"

template<class internal_t>
class directory_echo_access_t;

typedef int directory_echo_version_t;

template<class internal_t>
class directory_echo_wrapper_t {
public:
    directory_echo_wrapper_t() { }
    directory_echo_wrapper_t(internal_t i, directory_echo_version_t v, const mailbox_addr_t<void(peer_id_t, directory_echo_version_t)> &am) :
        internal(i), version(v), ack_mailbox(am) { }

    internal_t internal;
    directory_echo_version_t version;
    mailbox_addr_t<void(peer_id_t, directory_echo_version_t)> ack_mailbox;

    RDB_MAKE_ME_SERIALIZABLE_3(internal, version, ack_mailbox);
};

template<class internal_t>
class directory_echo_access_t {
public:
    class our_value_change_t {
    public:
        explicit our_value_change_t(directory_echo_access_t *p);
        directory_echo_version_t commit();
    private:
        directory_echo_access_t *parent;
        directory_write_service_t::our_value_lock_acq_t acq;
    public:
        internal_t buffer;
    };

    class ack_waiter_t : public signal_t {
    public:
        ack_waiter_t(directory_echo_access_t *parent, peer_id_t peer, directory_echo_version_t version);
    private:
        friend class directory_echo_access_t;
        boost::scoped_ptr<multimap_insertion_sentry_t<directory_echo_version_t, ack_waiter_t *> > map_entry;
    };

    directory_echo_access_t(
            mailbox_manager_t *mm,
            clone_ptr_t<directory_rwview_t<boost::optional<directory_echo_wrapper_t<internal_t> > > > mv,
            const internal_t &initial);

    ~directory_echo_access_t();

    clone_ptr_t<directory_rview_t<boost::optional<internal_t> > > get_internal_view();

private:
    void on_connect(peer_id_t peer);

    void on_disconnect(peer_id_t peer);

    void on_change(peer_id_t peer);

    void ack_version(mailbox_addr_t<void(peer_id_t, directory_echo_version_t)> peer, directory_echo_version_t version, auto_drainer_t::lock_t);

    void on_ack(peer_id_t peer, directory_echo_version_t version, auto_drainer_t::lock_t);

    mailbox_manager_t *mailbox_manager;
    clone_ptr_t<directory_rwview_t<boost::optional<directory_echo_wrapper_t<internal_t> > > > metadata_view;

    directory_echo_version_t our_version;
    std::map<peer_id_t, directory_echo_version_t> last_seen, last_acked;
    std::map<peer_id_t, std::multimap<directory_echo_version_t, ack_waiter_t *> > waiters;

    mutex_assertion_t ack_lock;

    auto_drainer_t drainer;
    mailbox_t<void(peer_id_t, directory_echo_version_t)> ack_mailbox;
    boost::ptr_map<peer_id_t, directory_read_service_t::peer_value_subscription_t> peer_value_subscriptions;
    connectivity_service_t::peers_list_subscription_t peers_list_subscription;
};

#endif /* CLUSTERING_REACTOR_DIRECTORY_ECHO_HPP_ */

#include "clustering/reactor/directory_echo.tcc"
