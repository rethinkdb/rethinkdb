#include <boost/make_shared.hpp>

/* This unholy monstrosity will go away when directory_rview_t does. */

template<class metadata_t>
class read_view_translator_t {
public:
    read_view_translator_t(const clone_ptr_t<directory_rview_t<metadata_t> > &_view) :
        view(_view),
        peers_list_sub(
            boost::bind(&read_view_translator_t<metadata_t>::on_connect, this, _1),
            boost::bind(&read_view_translator_t<metadata_t>::on_disconnect, this, _1))
    {
        connectivity_service_t::peers_list_freeze_t freeze(view->get_directory_service()->get_connectivity_service());
        peers_list_sub.reset(view->get_directory_service()->get_connectivity_service(), &freeze);
        std::set<peer_id_t> peers = view->get_directory_service()->get_connectivity_service()->get_peers_list();
        boost::ptr_map<peer_id_t, directory_read_service_t::peer_value_freeze_t> freezes;
        for (std::set<peer_id_t>::iterator it = peers.begin(); it != peers.end(); it++) {
            peer_id_t pid = *it;   /* workaround for Boost bug */
            freezes.insert(pid, new directory_read_service_t::peer_value_freeze_t(view->get_directory_service(), pid));
            peer_value_subs.insert(pid, new directory_read_service_t::peer_value_subscription_t(
                boost::bind(&read_view_translator_t<metadata_t>::on_value_change, this),
                view->get_directory_service(), pid, freezes.find(pid)->second
                ));
        }
        update();
    }
private:
    template<class md_t>
    friend class read_view_translator_watchable_t;
    static void call(const boost::function<void()> &fun) {
        fun();
    }
    void update() {
        rwi_lock_assertion_t::write_acq_t acq(&rwi_lock_assertion);
        value.clear();
        std::set<peer_id_t> peers = view->get_directory_service()->get_connectivity_service()->get_peers_list();
        for (std::set<peer_id_t>::iterator it = peers.begin(); it != peers.end(); it++) {
            value.insert(std::make_pair(*it, view->get_value(*it).get()));
        }
        publisher_controller.publish(&read_view_translator_t::call);
    }
    void on_connect(peer_id_t peer) {
        boost::ptr_map<peer_id_t, directory_read_service_t::peer_value_freeze_t> freezes;
        std::set<peer_id_t> peers = view->get_directory_service()->get_connectivity_service()->get_peers_list();
        for (std::set<peer_id_t>::iterator it = peers.begin(); it != peers.end(); it++) {
            peer_id_t pid = *it;   /* workaround for Boost bug */
            freezes.insert(pid, new directory_read_service_t::peer_value_freeze_t(view->get_directory_service(), *it));
        }
        peer_value_subs.insert(peer, new directory_read_service_t::peer_value_subscription_t(
            boost::bind(&read_view_translator_t<metadata_t>::on_value_change, this),
            view->get_directory_service(), peer, freezes.find(peer)->second
            ));
        update();
    }
    void on_disconnect(peer_id_t peer) {
        peer_value_subs.erase(peer);
        boost::ptr_map<peer_id_t, directory_read_service_t::peer_value_freeze_t> freezes;
        std::set<peer_id_t> peers = view->get_directory_service()->get_connectivity_service()->get_peers_list();
        for (std::set<peer_id_t>::iterator it = peers.begin(); it != peers.end(); it++) {
            peer_id_t pid = *it;   /* workaround for Boost bug */
            freezes.insert(pid, new directory_read_service_t::peer_value_freeze_t(view->get_directory_service(), *it));
        }
        update();
    }
    void on_value_change() {
        connectivity_service_t::peers_list_freeze_t freeze(view->get_directory_service()->get_connectivity_service());
        update();
    }
    clone_ptr_t<directory_rview_t<metadata_t> > view;
    std::map<peer_id_t, metadata_t> value;
    rwi_lock_assertion_t rwi_lock_assertion;
    publisher_controller_t<boost::function<void()> > publisher_controller;
    connectivity_service_t::peers_list_subscription_t peers_list_sub;
    boost::ptr_map<peer_id_t, directory_read_service_t::peer_value_subscription_t> peer_value_subs;
};

template<class metadata_t>
class read_view_translator_watchable_t : public watchable_t<std::map<peer_id_t, metadata_t> > {
public:
    read_view_translator_watchable_t(const boost::shared_ptr<read_view_translator_t<metadata_t> > &p) :
        parent(p) { }
    read_view_translator_watchable_t *clone() {
        return new read_view_translator_watchable_t(parent);
    }
    std::map<peer_id_t, metadata_t> get() {
        return parent->value;
    }
    publisher_t<boost::function<void()> > *get_publisher(typename watchable_t<std::map<peer_id_t, metadata_t> >::freeze_t *f) {
        f->assert_is_holding(clone_ptr_t<watchable_t<std::map<peer_id_t, metadata_t> > >(clone()));
        return parent->publisher_controller.get_publisher();
    }
    rwi_lock_assertion_t *get_rwi_lock_assertion() {
        return &parent->rwi_lock_assertion;
    }
private:
    boost::shared_ptr<read_view_translator_t<metadata_t> > parent;
};

template<class metadata_t>
clone_ptr_t<watchable_t<std::map<peer_id_t, metadata_t> > > translate_into_watchable(
    const clone_ptr_t<directory_rview_t<metadata_t> > &view) {

    boost::shared_ptr<read_view_translator_t<metadata_t> > translator =
        boost::make_shared<read_view_translator_t<metadata_t> >(view);
    return clone_ptr_t<watchable_t<std::map<peer_id_t, metadata_t> > >(
        new read_view_translator_watchable_t<metadata_t>(translator)
        );
}

template<class metadata_t>
clone_ptr_t<watchable_t<std::map<peer_id_t, metadata_t> > > translate_into_watchable(
    const clone_ptr_t<directory_rwview_t<metadata_t> > &view) {

    return translate_into_watchable(clone_ptr_t<directory_rview_t<metadata_t> >(view));
}
