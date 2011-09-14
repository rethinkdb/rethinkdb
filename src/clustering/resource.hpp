#ifndef __CLUSTERING_RESOURCE_HPP__
#define __CLUSTERING_RESOURCE_HPP__

#include "concurrency/wait_any.hpp"
#include "rpc/mailbox/mailbox.hpp"
#include "rpc/mailbox/typed.hpp"
#include "rpc/metadata/view.hpp"

/* It's a common paradigm to have some resource that you find out about through
metadata, and need to monitor in case it goes offline. These types facilitate
that. */

template<class business_card_t>
class resource_metadata_t;

template<class business_card_t>
void semilattice_join(resource_metadata_t<business_card_t> *, const resource_metadata_t<business_card_t>);

template<class business_card_t>
class resource_advertisement_t;

template<class business_card_t>
class resource_access_t;

/* `resource_metadata_t` is the type that you put in the metadata.
`business_card_t` is a type that contains information about the resource in
particular. Initially, you should set the resource's state to `unconstructed`.
*/

template<class business_card_t>
class resource_metadata_t {

public:
    resource_metadata_t() : state(unconstructed) { }

private:
    friend void semilattice_join<>(resource_metadata_t<business_card_t> *, const resource_metadata_t<business_card_t>);
    friend class resource_advertisement_t<business_card_t>;
    friend class resource_access_t<business_card_t>;

    enum state_t {
        unconstructed = 0,
        alive = 1,
        destroyed = 2
    };

    resource_metadata_t(mailbox_cluster_t *cluster, const business_card_t &ci) :
        state(destroyed), peer(cluster->get_me()), contact_info(ci) { }

    state_t state;
    peer_id_t peer;
    business_card_t contact_info;
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
exists. The constructor changes the state of the resource from `unconstructed`
to `alive`, and populates the `contact_info` field. The destructor changes the
state from `alive` to `destroyed`. */

template<class business_card_t>
class resource_advertisement_t {

public:
    resource_advertisement_t(
            mailbox_cluster_t *cluster,
            boost::shared_ptr<metadata_readwrite_view_t<resource_metadata_t<business_card_t> > > md_view,
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
    boost::shared_ptr<metadata_readwrite_view_t<resource_metadata_t<business_card_t> > > metadata_view;
};

/* A resource-user constructs a `resource_access_t` when it wants to start using
the resource. The `resource_access_t` will give it access to the resource's
contact info, but also ensure safety by watching to see if the resource goes
down. */

class resource_lost_exc_t : public std::exception {
public:
    resource_lost_exc_t() { }
    const char *what() const throw () {
        return "access to resource interrupted";
    }
};

template<class business_card_t>
class resource_access_t {

public:
    /* Starts monitoring the resource. Throws `resource_lost_exc_t` if the
    resource is inaccessible. */
    resource_access_t(
            mailbox_cluster_t *cluster,
            boost::shared_ptr<metadata_read_view_t<resource_metadata_t<business_card_t> > > v) :
        view(v),
        subs(boost::bind(&resource_access_t::check_dead, this))
    {
        /* If the resource sets its state to offline, then it's lost */
        check_dead();
        subs.resubscribe(view);
        either_failed.add(&resource_went_offline);

        /* If the node that the resource is on ceases to be visible, then it's
        lost */
        if (view->get().state != resource_metadata_t<business_card_t>::destroyed) {
            disconnect_watcher.reset(new disconnect_watcher_t(cluster, view->get().peer));
            either_failed.add(disconnect_watcher.get());
        }

        /* Throw an exception if something's gone wrong already */
        access();
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

    boost::shared_ptr<metadata_read_view_t<resource_metadata_t<business_card_t> > > view;

    typename metadata_read_view_t<resource_metadata_t<business_card_t> >::subscription_t subs;
    cond_t resource_went_offline;

    boost::scoped_ptr<disconnect_watcher_t> disconnect_watcher;

    wait_any_t either_failed;
};

#endif /* __CLUSTERING_RESOURCE_HPP__ */
