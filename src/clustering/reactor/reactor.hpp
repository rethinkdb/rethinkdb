#ifndef __CLUSTERING_REACTOR_REACTOR_HPP__
#define __CLUSTERING_REACTOR_REACTOR_HPP__

#include "clustering/reactors/metadata.hpp"
#include "clustering/reactors/delayed_pump.hpp"

template<class protocol_t>
class reactor_t {
public:
    reactor_t(
            mailbox_manager_t *mailbox_manager,
            clone_ptr_t<directory_readwrite_view_t<boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > > > reactor_directory,
            boost::shared_ptr<semilattice_read_view_t<branch_history_t<protocol_t> > > branch_history,
            watchable_t<blueprint_t<protocol_t> > *blueprint,
            std::string file_path) THROWS_NOTHING;

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
        directory_entry_version_t update(typename reactor_business_card_t<protocol_t>::activity_t);
        ~directory_entry_t();
    private:
        reactor_t<protocol_t> *parent;
        protocol_t::region_t region;
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

    mailbox_manager_t *mailbox_manager;
    //clone_ptr_t<directory_readwrite_view_t<reactor_business_card_t<protocol_t> > > reactor_directory;
    directory_echo_access_t<reactor_business_card_t<protocol_t> > directory_echo_access;
    boost::shared_ptr<semilattice_read_view_t<branch_history_t<protocol_t> > > branch_history;
    watchable_t<blueprint_t<protocol_t> > *blueprint;

    typename protocol_t::store_file_t store_file;

    std::map<
            typename protocol_t::region_t,
            std::pair<typename blueprint_t<protocol_t>::role_t, cond_t *> 
            > current_roles;

    auto_drainer_t drainer;

    watchable_subscription_t blueprint_subscription;
};

#endif /* __CLUSTERING_REACTOR_REACTOR_HPP__ */
