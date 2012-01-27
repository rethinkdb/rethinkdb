#ifndef __CLUSTERING_REACTOR_REACTOR_HPP__
#define __CLUSTERING_REACTOR_REACTOR_HPP__

#include "clustering/reactors/metadata.hpp"
#include "clustering/reactors/delayed_pump.hpp"

template<class protocol_t>
class reactor_t {
public:
    reactor_t(
            mailbox_manager_t *mailbox_manager,
            clone_ptr_t<directory_readwrite_view_t<reactor_business_card_t<protocol_t> > > reactor_directory,
            boost::shared_ptr<semilattice_read_view_t<branch_history_t<protocol_t> > > branch_history,
            watchable_t<blueprint_t<protocol_t> > *blueprint,
            std::string file_path) THROWS_NOTHING;

private:
    /* To save typing */
    peer_id_t get_me() THROWS_NOTHING{
        return mailbox_manager->get_connectivity_service()->get_me();
    }

    void on_new_blueprint() THROWS_NOTHING;

    void be_primary(typename protocol_t::region_t region, boost::shared_ptr<cond_t> blueprint_changed, auto_drainer_t::lock_t keepalive) THROWS_NOTHING;
    void be_secondary(typename protocol_t::region_t region, boost::shared_ptr<cond_t> blueprint_changed, auto_drainer_t::lock_t keepalive) THROWS_NOTHING;
    void be_nothing(typename protocol_t::region_t region, boost::shared_ptr<cond_t> blueprint_changed, auto_drainer_t::lock_t keepalive) THROWS_NOTHING;

    mailbox_manager_t *mailbox_manager;
    clone_ptr_t<directory_readwrite_view_t<reactor_business_card_t<protocol_t> > > reactor_directory;
    boost::shared_ptr<semilattice_read_view_t<branch_history_t<protocol_t> > > branch_history;
    watchable_t<blueprint_t<protocol_t> > *blueprint;

    typename protocol_t::store_file_t store_file;

    std::map<typename protocol_t::region_t, std::pair<typename blueprint_t<protocol_t>::role_t, boost::shared_ptr<cond_t> > > current_activities;

    auto_drainer_t drainer;

    watchable_subscription_t blueprint_subscription;
};

#endif /* __CLUSTERING_REACTOR_REACTOR_HPP__ */
