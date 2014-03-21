// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_REACTOR_DIRECTORY_ECHO_HPP_
#define CLUSTERING_REACTOR_DIRECTORY_ECHO_HPP_

#include <map>

#include "concurrency/watchable.hpp"
#include "containers/scoped.hpp"
#include "containers/incremental_lenses.hpp"
#include "rpc/mailbox/typed.hpp"
#include "rpc/semilattice/joins/macros.hpp"
#include "utils.hpp"

typedef int32_t directory_echo_version_t;

template<class internal_t>
class directory_echo_wrapper_t {
public:
    directory_echo_wrapper_t() { }
    internal_t internal;
private:
    template<class internal2_t>
    friend class directory_echo_writer_t;
    template<class internal2_t>
    friend class directory_echo_mirror_t;
    directory_echo_wrapper_t(const internal_t &i, directory_echo_version_t v, const mailbox_addr_t<void(peer_id_t, directory_echo_version_t)> &am) :
        internal(i), version(v), ack_mailbox(am) { }
    directory_echo_version_t version;
    mailbox_addr_t<void(peer_id_t, directory_echo_version_t)> ack_mailbox;
public:
    RDB_MAKE_ME_SERIALIZABLE_3(internal, version, ack_mailbox);
    RDB_MAKE_ME_EQUALITY_COMPARABLE_3(directory_echo_wrapper_t<internal_t>,
        internal, version, ack_mailbox);
};

template<class internal_t>
class directory_echo_writer_t {
public:
    class our_value_change_t {
    public:
        explicit our_value_change_t(directory_echo_writer_t *p);
        directory_echo_version_t commit();

        // field ordering matters here?
    private:
        directory_echo_writer_t *parent;
        mutex_assertion_t::acq_t lock_acq;

    public:
        internal_t buffer;

    private:
        DISABLE_COPYING(our_value_change_t);
    };

    class ack_waiter_t : public signal_t {
    public:
        ack_waiter_t(directory_echo_writer_t *parent, peer_id_t peer, directory_echo_version_t version);
    private:
        friend class directory_echo_writer_t;
        scoped_ptr_t<multimap_insertion_sentry_t<directory_echo_version_t, ack_waiter_t *> > map_entry;

        DISABLE_COPYING(ack_waiter_t);
    };

    directory_echo_writer_t(
            mailbox_manager_t *mm,
            const internal_t &initial);

    clone_ptr_t<watchable_t<directory_echo_wrapper_t<internal_t> > > get_watchable() {
        return value_watchable.get_watchable();
    }

private:
    void on_ack(peer_id_t peer, directory_echo_version_t version, auto_drainer_t::lock_t);

    std::map<peer_id_t, directory_echo_version_t> last_acked;
    std::map<peer_id_t, std::multimap<directory_echo_version_t, ack_waiter_t *> > waiters;
    mutex_assertion_t ack_lock;

    auto_drainer_t drainer;
    mailbox_t<void(peer_id_t, directory_echo_version_t)> ack_mailbox;

    watchable_variable_t<directory_echo_wrapper_t<internal_t> > value_watchable;
    mutex_assertion_t value_lock;
    directory_echo_version_t version;

    DISABLE_COPYING(directory_echo_writer_t);
};

template<class internal_t>
class directory_echo_mirror_t {
public:
    directory_echo_mirror_t(
            mailbox_manager_t *mm,
            const clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, directory_echo_wrapper_t<internal_t> > > > &peers);

    /* Returns a view to the watchable with the `directory_echo_wrapper_t`s
    stripped away. As an added benefit, if the outer watchable publishes a
    spurious event that doesn't change the value, this won't publish an event.
    We can do that by checking the version numbers to see if they have changed
    and thereby detect the spurious event. */
    clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, internal_t> > > get_internal() {
        return subview.get_watchable();
    }

private:
    void on_change();
    void ack_version(
            mailbox_t<void(peer_id_t, directory_echo_version_t)>::address_t peer,
            directory_echo_version_t version, auto_drainer_t::lock_t);
    mailbox_manager_t *mailbox_manager;
    clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, directory_echo_wrapper_t<internal_t> > > > peers;
    std::map<peer_id_t, directory_echo_version_t> last_seen;

    change_tracking_map_t<peer_id_t, internal_t> subview_value;
    watchable_variable_t<change_tracking_map_t<peer_id_t, internal_t> > subview;

    auto_drainer_t drainer;
    typename watchable_t<change_tracking_map_t<peer_id_t, directory_echo_wrapper_t<internal_t> > >::subscription_t sub;

    DISABLE_COPYING(directory_echo_mirror_t);
};

#endif /* CLUSTERING_REACTOR_DIRECTORY_ECHO_HPP_ */

#include "clustering/reactor/directory_echo.tcc"
