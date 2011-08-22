#ifndef __CLUSTERING_RESOURCE_HPP__
#define __CLUSTERING_RESOURCE_HPP__

#include "concurrency/wait_any.hpp"

/* It's a common paradigm to have some resource that you find out about through
metadata, and need to monitor in case it goes offline. These types facilitate
that. */

template<class business_card_t>
class resource_metadata_t {
public:
    enum state_t {
        unconstructed = 0,
        alive = 1,
        destroyed = 2
    } state;
    peer_id_t peer;
    business_card_t contact_info;

    resource_metadata_t() : state(unconstructed) { }

    resource_metadata_t(mailbox_cluster_t *cluster, const business_card_t &ci) :
        state(destroyed), peer(cluster->get_me()), contact_info(ci) { }
};

template<class business_card_t>
void semilattice_join(resource_metadata_t<business_card_t> *a, const resource_metadata_t<business_card_t> b) {
    if (b.state == a->state && a->state == resource_metadata_t<business_card_t>::alive) {
        rassert(a->peer == b.peer);
        semilattice_join(&a->contact_info, b.contact_info);
    } else if (b.state > a->state) {
        *a = b;
    }
}

/* A resource constructs a `resource_advertisement_t` to tell everybody that it
exists. */

template<class business_card_t>
class resource_advertisement_t {

public:
    resource_advertisement_t(
            mailbox_cluster_t *cluster,
            metadata_readwrite_view_t<resource_metadata_t<business_card_t> > *md_view,
            const business_card_t &initial) :
        metadata_view(md_view)
    {
        rassert(metadata_view->get().state == resource_metadata_t<business_card_t>::unconstructed);

        resource_metadata_t<business_card_t> res_md;
        res_md.state = resource_metadata_t<business_card_t>::alive;
        res_md.peer = cluster->get_me();
        res_md.contact_info = initial;
        metadata_view->join(res_md);
    }

    ~resource_advertisement_t() {
        rassert(metadata_view->get().state == resource_metadata_t<business_card_t>::alive);

        resource_metadata_t<business_card_t> res_md;
        res_md.state = resource_metadata_t<business_card_t>::destroyed;
        metadata_view->join(res_md);
    }

private:
    metadata_readwrite_view_t<resource_metadata_t<business_card_t> > *metadata_view;
};

class resource_lost_exc_t : public std::exception {
public:
    resource_lost_exc_t() { }
    const char *what() const throw () {
        return "access to resource interrupted";
    }
};

/* A resource-user constructs a `resource_access_t` when it wants to start using
the resource. The `resource_access_t` will allow it to use the resource but
ensure safety by throwing an exception if the resource dies. */

template<class business_card_t>
class resource_access_t {

public:
    /* Starts monitoring the resource. Throws `resource_lost_exc_t` if the
    resource is inaccessible. */
    resource_access_t(
            mailbox_cluster_t *cluster,
            metadata_read_view_t<resource_metadata_t<business_card_t> > *view) :
        view(view),
        subs(boost::bind(&resource_access_t::check_dead, this)),
        disconnect_watcher(cluster, view->get().peer),
        either_failed(&resource_went_offline, &disconnect_watcher)
    {
        check_dead();
        subs.resubscribe(view);
        access();   // To crash if something is already wrong
    }

    /* Returns a `signal_t *` that will be pulsed when access to the resource is
    lost. */
    signal_t *get_failed_signal() {
        return &either_failed;
    }

    /* Returns the resource contact info if the resource is accessible. Throws
    `resource_lost_exc_t` if it's not. */
    business_card_t access() {
        if (either_failed.is_pulsed()) throw resource_lost_exc_t();
        return view->get().contact_info;
    }

private:
    void check_dead() {
        switch (view->get().state) {
            case resource_metadata_t<business_card_t>::unconstructed:
                crash("WTF: Trying to use unconstructed resource");
            case resource_metadata_t<business_card_t>::alive:
                return;
            case resource_metadata_t<business_card_t>::destroyed:
                if (!resource_went_offline.is_pulsed()) {
                    resource_went_offline.pulse();
                }
                break;
            default:
                unreachable();
        }
    }

    metadata_read_view_t<resource_metadata_t<business_card_t> > *view;

    typename metadata_read_view_t<resource_metadata_t<business_card_t> >::subscription_t subs;
    cond_t resource_went_offline;

    disconnect_watcher_t disconnect_watcher;

    wait_any_t either_failed;
};

#endif /* __CLUSTERING_RESOURCE_HPP__ */
