#include "errors.hpp"
#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/scoped_ptr.hpp>

#include "concurrency/cond_var.hpp"
#include "rpc/connectivity/connectivity.hpp"
#include "concurrency/wait_any.hpp"

template<class metadata_t, class inner_t>
class subview_directory_single_rview_t :
    public directory_single_rview_t<inner_t>
{
public:
    subview_directory_single_rview_t(
            directory_single_rview_t<metadata_t> *clone_me,
            const clone_ptr_t<read_lens_t<inner_t, metadata_t> > &l) :
        superview(clone_me->clone()), lens(l)
        { }

    ~subview_directory_single_rview_t() THROWS_NOTHING { }

    subview_directory_single_rview_t *clone() const THROWS_NOTHING {
        return new subview_directory_single_rview_t(superview.get(), lens);
    }

    boost::optional<inner_t> get_value() THROWS_NOTHING {
        boost::optional<metadata_t> outer = superview->get_value();
        if (outer) {
            return boost::optional<inner_t>(lens->get(outer.get()));
        } else {
            return boost::optional<inner_t>();
        }
    }

    directory_read_service_t *get_directory_service() THROWS_NOTHING {
        return superview->get_directory_service();
    }

    peer_id_t get_peer() THROWS_NOTHING {
        return superview->get_peer();
    }

private:
    boost::scoped_ptr<directory_single_rview_t<metadata_t> > superview;
    clone_ptr_t<read_lens_t<inner_t, metadata_t> > lens;
};

template<class metadata_t> template<class inner_t>
clone_ptr_t<directory_single_rview_t<inner_t> > directory_single_rview_t<metadata_t>::subview(const clone_ptr_t<read_lens_t<inner_t, metadata_t> > &lens) THROWS_NOTHING {
    return clone_ptr_t<directory_single_rview_t<inner_t> >(
        new subview_directory_single_rview_t<metadata_t, inner_t>(
            this,
            lens
        ));
}

template<class metadata_t, class inner_t>
class subview_directory_rview_t :
    public directory_rview_t<inner_t>
{
public:
    subview_directory_rview_t(
            directory_rview_t<metadata_t> *clone_me,
            const clone_ptr_t<read_lens_t<inner_t, metadata_t> > &l) :
        superview(clone_me->clone()), lens(l)
        { }

    ~subview_directory_rview_t() THROWS_NOTHING { }

    subview_directory_rview_t *clone() const THROWS_NOTHING {
        return new subview_directory_rview_t(superview.get(), lens);
    }

    boost::optional<inner_t> get_value(peer_id_t peer) THROWS_NOTHING {
        boost::optional<metadata_t> outer = superview->get_value(peer);
        if (outer) {
            return boost::optional<inner_t>(lens->get(outer.get()));
        } else {
            return boost::optional<inner_t>();
        }
    }

    directory_read_service_t *get_directory_service() THROWS_NOTHING {
        return superview->get_directory_service();
    }

private:
    boost::scoped_ptr<directory_rview_t<metadata_t> > superview;
    clone_ptr_t<read_lens_t<inner_t, metadata_t> > lens;
};

template<class metadata_t> template<class inner_t>
clone_ptr_t<directory_rview_t<inner_t> > directory_rview_t<metadata_t>::subview(const clone_ptr_t<read_lens_t<inner_t, metadata_t> > &lens) THROWS_NOTHING {
    return clone_ptr_t<directory_rview_t<inner_t> >(
        new subview_directory_rview_t<metadata_t, inner_t>(
            this,
            lens
        ));
}

template<class metadata_t>
class peer_subview_directory_single_rview_t :
    public directory_single_rview_t<metadata_t>
{
public:
    peer_subview_directory_single_rview_t(
            directory_rview_t<metadata_t> *clone_me,
            peer_id_t p) :
        superview(clone_me->clone()), peer(p)
        { }

    ~peer_subview_directory_single_rview_t() THROWS_NOTHING { }

    peer_subview_directory_single_rview_t *clone() const THROWS_NOTHING {
        return new peer_subview_directory_single_rview_t(superview.get(), peer);
    }

    boost::optional<metadata_t> get_value() THROWS_NOTHING {
        return superview->get_value(peer);
    }

    directory_read_service_t *get_directory_service() THROWS_NOTHING {
        return superview->get_directory_service();
    }

    peer_id_t get_peer() THROWS_NOTHING {
        return peer;
    }

private:
    boost::scoped_ptr<directory_rview_t<metadata_t> > superview;
    peer_id_t peer;
};

inline void pulse_if_not_pulsed(cond_t *cond) {
    if (!cond->is_pulsed()) {
        cond->pulse();
    }
}

inline void pulse_if_not_pulsed_and_remove_subscription(cond_t *cond, peer_id_t peer_id, boost::ptr_map<peer_id_t, directory_read_service_t::peer_value_subscription_t> *subscription_map) {
    pulse_if_not_pulsed(cond);

    subscription_map->erase(peer_id);
}

inline void pulse_if_not_pulsed_and_clear_subscription(cond_t *cond, peer_id_t peer_that_disconnected, peer_id_t relevant_peer, directory_read_service_t::peer_value_subscription_t *subscription) {
    if (peer_that_disconnected == relevant_peer) {
        pulse_if_not_pulsed(cond);
        subscription->reset();
    }
}

template<class metadata_t>
void directory_single_rview_t<metadata_t>::run_until_satisfied(const boost::function<bool(boost::optional<metadata_t>)> &f, signal_t *interruptor) {
    while (true) {
        boost::scoped_ptr<connectivity_service_t::peers_list_freeze_t> peers_list_freeze(new connectivity_service_t::peers_list_freeze_t(get_directory_service()->get_connectivity_service()));

        if (get_directory_service()->get_connectivity_service()->get_peer_connected(get_peer())) {
            boost::scoped_ptr<directory_read_service_t::peer_value_freeze_t> peer_value_freeze(new directory_read_service_t::peer_value_freeze_t(get_directory_service(), get_peer()));

            if (f(get_value())) {
                return;
            }

            cond_t something_has_changed;

            directory_read_service_t::peer_value_subscription_t peer_subscription(boost::bind(&pulse_if_not_pulsed, &something_has_changed),
                                                                                  get_directory_service(),
                                                                                  get_peer(),
                                                                                  peer_value_freeze.get());

            connectivity_service_t::peers_list_subscription_t peers_list_subscription(NULL,
                                                                                      boost::bind(&pulse_if_not_pulsed_and_clear_subscription, &something_has_changed, _1, get_peer(), &peer_subscription),
                                                                                      get_directory_service()->get_connectivity_service(), peers_list_freeze.get());

            peer_value_freeze.reset();
            peers_list_freeze.reset();

            wait_interruptible(&something_has_changed, interruptor);
        } else {
            if (f(get_value())) {
                return;
            }

            connect_watcher_t connect_watcher(get_directory_service()->get_connectivity_service(), get_peer());
            wait_interruptible(&connect_watcher, interruptor);
        }
    }
}

template<class metadata_t>
void directory_rview_t<metadata_t>::run_until_satisfied(const boost::function<bool(std::map<peer_id_t, metadata_t>)> &f, signal_t *interruptor) {
    while (true) {
        boost::scoped_ptr<connectivity_service_t::peers_list_freeze_t> peers_list_freeze(new connectivity_service_t::peers_list_freeze_t(get_directory_service()->get_connectivity_service()));
        std::set<peer_id_t> peers = get_directory_service()->get_connectivity_service()->get_peers_list();

        boost::ptr_map<peer_id_t, directory_read_service_t::peer_value_freeze_t> peer_value_freezes;
        std::map<peer_id_t, metadata_t> directory_value;

        for (std::set<peer_id_t>::iterator it =  peers.begin();
                                           it != peers.end();
                                           it++) {
            //tmp is a workaround for a bug in boost
            //https://svn.boost.org/trac/boost/ticket/6089
            peer_id_t tmp = *it;
            peer_value_freezes.insert(tmp, new directory_read_service_t::peer_value_freeze_t(get_directory_service(), *it));
            //The value should always exist because we got this peer from the
            //peers list. If it doesn't this indicates a programmer error in
            //directory code.
            directory_value[*it] = *get_value(*it);
        }
        if (f(directory_value)) {
            return;
        }

        cond_t something_has_changed;
        boost::ptr_map<peer_id_t, directory_read_service_t::peer_value_subscription_t> subscription_map;

        connectivity_service_t::peers_list_subscription_t peers_list_subscription(boost::bind(&pulse_if_not_pulsed, &something_has_changed),
                                                                                  boost::bind(&pulse_if_not_pulsed_and_remove_subscription, &something_has_changed, _1, &subscription_map),
                                                                                  get_directory_service()->get_connectivity_service(), peers_list_freeze.get());

        for (std::set<peer_id_t>::iterator it =  peers.begin();
                                           it != peers.end();
                                           it++) {
            //tmp is a workaround for a bug in boost
            //https://svn.boost.org/trac/boost/ticket/6089
            peer_id_t tmp = *it;
            subscription_map.insert(tmp, new directory_read_service_t::peer_value_subscription_t(boost::bind(&pulse_if_not_pulsed, &something_has_changed), get_directory_service(), *it, peer_value_freezes.find(*it)->second));
        }
        peer_value_freezes.clear();
        peers_list_freeze.reset();

        wait_interruptible(&something_has_changed, interruptor);
    }
}

template<class metadata_t>
clone_ptr_t<directory_single_rview_t<metadata_t> > directory_rview_t<metadata_t>::get_peer_view(peer_id_t peer) THROWS_NOTHING {
    return clone_ptr_t<directory_single_rview_t<metadata_t> >(
        new peer_subview_directory_single_rview_t<metadata_t>(
            this,
            peer
        ));
}
