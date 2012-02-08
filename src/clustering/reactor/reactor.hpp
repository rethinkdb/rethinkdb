#ifndef __CLUSTERING_REACTOR_REACTOR_HPP__
#define __CLUSTERING_REACTOR_REACTOR_HPP__

#include "clustering/reactor/metadata.hpp"
#include "rpc/directory/view.hpp"
#include "clustering/reactor/directory_echo.hpp"
#include "clustering/immediate_consistency/query/metadata.hpp"

template<class protocol_t>
class reactor_t {
public:
    reactor_t(
            mailbox_manager_t *mailbox_manager,
            clone_ptr_t<directory_rwview_t<boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > > > reactor_directory,
            clone_ptr_t<directory_wview_t<std::map<master_id_t, master_business_card_t<protocol_t> > > > _master_directory,
            boost::shared_ptr<semilattice_readwrite_view_t<branch_history_t<protocol_t> > > branch_history,
            watchable_t<blueprint_t<protocol_t> > *blueprint,
            store_view_t<protocol_t> *_underlying_store) THROWS_NOTHING;

private:
    /* a directory_entry_t is a sentry that in its contructor inserts an entry
     * into the directory for a role that we are performing (a role that we
     * have spawned a coroutine to perform). In its destructor it removes that
     * entry.
     */
    class directory_entry_t {
    public:
        directory_entry_t(reactor_t<protocol_t> *, typename protocol_t::region_t);

        //Changes just the activity, leaves everything else in tact
        directory_echo_version_t set(typename reactor_business_card_t<protocol_t>::activity_t);

        //XXX this is a bit of a hack that we should revisit when we know more
        directory_echo_version_t update_without_changing_id(typename reactor_business_card_t<protocol_t>::activity_t);
        ~directory_entry_t();

        reactor_activity_id_t get_reactor_activity_id() {
            return reactor_activity_id;
        }
    private:
        reactor_t<protocol_t> *parent;
        typename protocol_t::region_t region;
        reactor_activity_id_t reactor_activity_id;
    };

    /* To save typing */
    peer_id_t get_me() THROWS_NOTHING{
        return mailbox_manager->get_connectivity_service()->get_me();
    }

    void on_blueprint_changed() THROWS_NOTHING;
    void try_spawn_roles() THROWS_NOTHING;
    void run_role(
            typename protocol_t::region_t region,
            typename blueprint_t<protocol_t>::role_t role,
            cond_t *blueprint_changed_cond,
            auto_drainer_t::lock_t keepalive) THROWS_NOTHING;

    void be_primary(typename protocol_t::region_t region, store_view_t<protocol_t> *store,
            signal_t *interruptor) THROWS_NOTHING;
    void be_secondary(typename protocol_t::region_t region, store_view_t<protocol_t> *store,
            signal_t *interruptor) THROWS_NOTHING;
    void be_listener(typename protocol_t::region_t region, store_view_t<protocol_t> *store,
            signal_t *interruptor) THROWS_NOTHING;
    void be_nothing(typename protocol_t::region_t region, store_view_t<protocol_t> *store,
            signal_t *interruptor) THROWS_NOTHING;


    void wait_for_directory_acks(directory_echo_version_t, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

    boost::optional<clone_ptr_t<directory_single_rview_t<boost::optional<broadcaster_business_card_t<protocol_t> > > > >
        find_broadcaster_in_directory(const typename protocol_t::region_t &) {
            crash("Not implemented\n");
    }

    clone_ptr_t<directory_single_rview_t<boost::optional<broadcaster_business_card_t<protocol_t> > > >
        wait_for_broadcaster_to_appear_in_directory(const typename protocol_t::region_t &, signal_t *) {
            crash("Not implemented\n");
    }


    template <class activity_t>
    clone_ptr_t<directory_single_rview_t<boost::optional<activity_t> > > get_directory_entry_view(peer_id_t id, const reactor_activity_id_t&);

    mailbox_manager_t *mailbox_manager;

    directory_echo_access_t<reactor_business_card_t<protocol_t> > directory_echo_access;
    clone_ptr_t<directory_wview_t<std::map<master_id_t, master_business_card_t<protocol_t> > > > master_directory;
    boost::shared_ptr<semilattice_readwrite_view_t<branch_history_t<protocol_t> > > branch_history;

    watchable_t<blueprint_t<protocol_t> > *blueprint;

    store_view_t<protocol_t> *underlying_store;

    std::map<
            typename protocol_t::region_t,
            std::pair<typename blueprint_t<protocol_t>::role_t, cond_t *> 
            > current_roles;

    auto_drainer_t drainer;

    typename watchable_t<blueprint_t<protocol_t> >::subscription_t blueprint_subscription;
};

#include "clustering/reactor/reactor.tcc"

#endif /* __CLUSTERING_REACTOR_REACTOR_HPP__ */
