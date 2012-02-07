#ifndef __CLUSTERING_REACTOR_DIRECTORY_ECHO_HPP__
#define __CLUSTERING_REACTOR_DIRECTORY_ECHO_HPP__

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
private:
    friend class directory_echo_access_t<internal_t>;
    directory_echo_wrapper_t(internal_t i, directory_echo_version_t v, const async_mailbox_t<void(peer_id_t, directory_echo_version_t)>::address_t &am) :
        internal(i), version(v), ack_mailbox(am) { }
    internal_t internal;
    directory_echo_version_t version;
    async_mailbox_t<void(peer_id_t, directory_echo_version_t)>::address_t ack_mailbox;
    RDB_MAKE_ME_SERIALIZABLE_3(internal, version, ack_mailbox);
};

template<class internal_t>
class directory_echo_access_t {
public:
    class our_value_change_t {
    public:
        explicit our_value_change_t(directory_echo_access_t *p) :
            parent(p), acq(parent->metadata_view->get_directory_service()),
            buffer(parent->metadata_view->get_our_value(&acq).get().internal)
            { }
        directory_echo_version_t commit() {
            parent->our_version++;
            parent->metadata_view->set_our_value(
                boost::optional<directory_echo_wrapper_t<internal_t> >(
                    directory_echo_wrapper_t<internal_t>(
                        buffer,
                        parent->our_version,
                        parent->ack_mailbox.get_address()
                        )
                    ),
                &acq
                );
            return parent->our_version;
        }
    private:
        directory_echo_access_t *parent;
        directory_write_service_t::our_value_lock_acq_t acq;
    public:
        internal_t buffer;
    };

    class ack_waiter_t : public signal_t {
    public:
        ack_waiter_t(directory_echo_access_t *parent, peer_id_t peer, directory_echo_version_t version) {
            mutex_assertion_t::acq_t acq(&parent->ack_lock);
            std::map<peer_id_t, directory_echo_version_t>::iterator it = parent->last_acked.find(peer);
            if (it != parent->last_acked.end() && (*it).second >= version) {
                pulse();
                return;
            }
            typename std::map<peer_id_t, std::multimap<directory_echo_version_t, ack_waiter_t *> >::iterator it2 =
                parent->waiters.find(peer);
            if (it2 == parent->waiters.end()) {
                it2 = parent->waiters.insert(std::make_pair(peer, std::multimap<directory_echo_version_t, ack_waiter_t *>())).first;
            }
            map_entry.reset(
                new multimap_insertion_sentry_t<directory_echo_version_t, ack_waiter_t *>(
                    &(*it2).second, version, this
                    )
                );
        }
    private:
        friend class directory_echo_access_t;
        boost::scoped_ptr<multimap_insertion_sentry_t<directory_echo_version_t, ack_waiter_t *> > map_entry;
    };

    directory_echo_access_t(
            mailbox_manager_t *mm,
            clone_ptr_t<directory_rwview_t<boost::optional<directory_echo_wrapper_t<internal_t> > > > mv,
            const internal_t &initial) :
        mailbox_manager(mm),
        metadata_view(mv),
        our_version(0),
        ack_mailbox(mailbox_manager, boost::bind(
            &directory_echo_access_t<internal_t>::on_ack, this,
            _1, _2, auto_drainer_t::lock_t(&drainer)
            )),
        peers_list_subscription(
            boost::bind(&directory_echo_access_t<internal_t>::on_connect, this, _1),
            boost::bind(&directory_echo_access_t<internal_t>::on_disconnect, this, _1)
            )
    {
        {
            connectivity_service_t::peers_list_freeze_t peers_list_freeze(
                metadata_view->get_directory_service()->get_connectivity_service());
            peers_list_subscription.reset(metadata_view->get_directory_service()->get_connectivity_service(), &peers_list_freeze);
            std::set<peer_id_t> peers_list = metadata_view->get_directory_service()->get_connectivity_service()->get_peers_list();
            for (std::set<peer_id_t>::iterator it = peers_list.begin(); it != peers_list.end(); it++) {
                on_connect(*it);
            }
        }
        {
            directory_write_service_t::our_value_lock_acq_t acq(metadata_view->get_directory_service());
            metadata_view->set_our_value(
                boost::optional<directory_echo_wrapper_t<internal_t> >(
                    directory_echo_wrapper_t<internal_t>(
                        initial,
                        our_version,
                        ack_mailbox.get_address()
                        )
                    ),
                &acq);
        }
    }

    ~directory_echo_access_t() {
        {
            directory_write_service_t::our_value_lock_acq_t acq(metadata_view->get_directory_service());
            metadata_view->set_our_value(boost::optional<directory_echo_wrapper_t<internal_t> >(), &acq);
        }
        {
            connectivity_service_t::peers_list_freeze_t peers_list_freeze(
                metadata_view->get_directory_service()->get_connectivity_service());
            peers_list_subscription.reset();
            std::set<peer_id_t> peers_list = metadata_view->get_directory_service()->
                get_connectivity_service()->get_peers_list();
            for (std::set<peer_id_t>::iterator it = peers_list.begin(); it != peers_list.end(); it++) {
                on_disconnect(*it);
            }
        }
    }

    clone_ptr_t<directory_rview_t<boost::optional<internal_t> > > get_internal_view() {
        return clone_ptr_t<directory_rview_t<boost::optional<directory_echo_wrapper_t<internal_t> > > >(metadata_view)->
            subview(
                optional_monad_lens(
                    clone_ptr_t<read_lens_t<internal_t, directory_echo_wrapper_t<internal_t> > >(
                        field_lens(&directory_echo_wrapper_t<internal_t>::internal)
                        )
                    )
                );
    }

private:
    void on_connect(peer_id_t peer) {
        directory_read_service_t::peer_value_freeze_t peer_value_freeze(metadata_view->get_directory_service(), peer);
        peer_value_subscriptions.insert(
            peer,
            new directory_read_service_t::peer_value_subscription_t(
                boost::bind(&directory_echo_access_t<internal_t>::on_change, this, peer),
                metadata_view->get_directory_service(),
                peer,
                &peer_value_freeze
                )
            );
        on_change(peer);
    }

    void on_disconnect(peer_id_t peer) {
        peer_value_subscriptions.erase(peer);
    }

    void on_change(peer_id_t peer) {
        boost::optional<boost::optional<directory_echo_wrapper_t<internal_t> > > value =
            metadata_view->get_value(peer);
        rassert(value, "on_change() for unconnected peer");
        if (value.get()) {
            int version = value.get().get().version;
            std::map<peer_id_t, directory_echo_version_t>::iterator it = last_seen.find(peer);
            if (it == last_seen.end() || (*it).second < version) {
                last_seen[peer] = version;
                coro_t::spawn_sometime(boost::bind(
                    &directory_echo_access_t<internal_t>::ack_version, this,
                    value.get().get().ack_mailbox, version,
                    auto_drainer_t::lock_t(&drainer)
                    ));
            }
        }
    }

    void ack_version(async_mailbox_t<void(peer_id_t, directory_echo_version_t)>::address_t peer, directory_echo_version_t version, auto_drainer_t::lock_t) {
        send(mailbox_manager, peer, mailbox_manager->get_connectivity_service()->get_me(), version);
    }

    void on_ack(peer_id_t peer, directory_echo_version_t version, auto_drainer_t::lock_t) {
        mutex_assertion_t::acq_t acq(&ack_lock);
        std::map<peer_id_t, directory_echo_version_t>::iterator it = last_acked.find(peer);
        if (it == last_acked.end() || (*it).second < version) {
            last_acked[peer] = version;
        }
        typename std::map<peer_id_t, std::multimap<directory_echo_version_t, ack_waiter_t *> >::iterator it2 =
            waiters.find(peer);
        if (it2 != waiters.end()) {
            for (typename std::multimap<directory_echo_version_t, ack_waiter_t *>::iterator it3 = (*it2).second.begin();
                    it3 != (*it2).second.upper_bound(version); it3++) {
                if (!(*it3).second->is_pulsed()) {
                    (*it3).second->pulse();
                }
            }
        }
    }

    mailbox_manager_t *mailbox_manager;
    clone_ptr_t<directory_rwview_t<boost::optional<directory_echo_wrapper_t<internal_t> > > > metadata_view;

    directory_echo_version_t our_version;
    std::map<peer_id_t, directory_echo_version_t> last_seen, last_acked;
    std::map<peer_id_t, std::multimap<directory_echo_version_t, ack_waiter_t *> > waiters;

    mutex_assertion_t ack_lock;

    auto_drainer_t drainer;
    async_mailbox_t<void(peer_id_t, directory_echo_version_t)> ack_mailbox;
    boost::ptr_map<peer_id_t, directory_read_service_t::peer_value_subscription_t> peer_value_subscriptions;
    connectivity_service_t::peers_list_subscription_t peers_list_subscription;
};

#endif /* __CLUSTERING_REACTOR_DIRECTORY_ECHO_HPP__ */
