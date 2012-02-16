#include "errors.hpp"
#include <boost/scoped_ptr.hpp>

#include "concurrency/cond_var.hpp"
#include "rpc/connectivity/connectivity.hpp"

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

    subview_directory_single_rview_t *clone() THROWS_NOTHING {
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

template <class metadata_t>
metadata_t directory_single_rview_t<metadata_t>::get_actual_value(signal_t *interruptor) {
    while (true) {
        boost::optional<metadata_t> val = get_value();
        if (val) {
            return val.get();
        }
        
        connect_watcher_t connect_watcher(get_directory_service()->get_connectivity_service(), get_peer());
        wait_interruptible(&connect_watcher, interruptor);
    }
}


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

    subview_directory_rview_t *clone() THROWS_NOTHING {
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

    peer_subview_directory_single_rview_t *clone() THROWS_NOTHING {
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
            peer_value_freezes.insert(*it, new directory_read_service_t::peer_value_freeze_t(get_directory_service(), *it));
            directory_value[*it] = get_value(*it);
        }
        if (f(directory_value)) {
            return;
        }

        cond_t something_has_changed;
        boost::ptr_map<peer_id_t, directory_read_service_t::peer_value_subscription_t> subscription_map;

        connectivity_service_t::peers_list_subscription_t peers_list_subscription(boost::bind(&pulse_if_not_pulsed, &cond), 
                                                                                  boost::bind(&pulse_if_not_pulsed_and_remove_subscription, &cond, _1, &subscription_map), 
                                                                                  get_directory_service()->get_connectivity_service(), peers_list_freeze.get());

        for (std::set<peer_id_t>::iterator it =  peers.begin();
                                           it != peers.end();
                                           it++) {
            subscription_map.insert(*it, new directory_read_service_t::peer_value_subscription_t(boost::bind(&pulse_if_not_pulsed, &cond), get_directory_service(), *it, &peer_value_freezes[*it]));
        }
        peer_value_freezes.clear();
        peers_list_freeze.reset();

        wait_interruptible(&cond, interruptor);
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
