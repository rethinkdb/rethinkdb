template<class protocol_t>
reactor_t<protocol_t>::reactor_t(
        mailbox_manager_t *mm,
        clone_ptr_t<directory_readwrite_view_t<reactor_business_card_t<protocol_t> > > rd,
        boost::shared_ptr<semilattice_read_view_t<branch_history_t<protocol_t> > > bh,
        watchable_t<blueprint_t<protocol_t> > *b,
        std::string file_path) THROWS_NOTHING :
    mailbox_manager(mm), reactor_directory(rd), branch_history(bh), blueprint(b),
    store_file(file_path),
    backfiller(mailbox_manager, branch_history, &store_file),
    blueprint_subscription(boost::bind(&reactor_t<protocol_t>::on_blueprint_changed, this))
{
    {
        watchable_freeze_t freeze(blueprint);
        try_spawn_activities();
        blueprint_subscription.reset(blueprint, &freeze);
    }
}

template<class protocol_t>
void reactor_t<protocol_t>::on_blueprint_changed() THROWS_NOTHING {
    std::map<typename protocol_t::region_t, typename blueprint_t<protocol_t>::role_t> blueprint_roles =
        (*blueprint->get().peers.find(get_me())).second;
    typename std::map<
            typename protocol_t::region_t,
            std::pair<typename blueprint_t<protocol_t>::role_t, cond_t *>
            >::iterator it;
    for (it = current_roles.begin(); it != current_roles.end(); it++) {
        typename std::map<typename protocol_t::region_t, typename blueprint_t<protocol_t>::role_t>::iterator it2 =
            blueprint_roles.find((*it).first);
        if (it2 == blueprint_roles.end() || (*it).second.first != (*it2).second) {
            if (!(*it).second.second->is_pulsed()) {
                (*it).second.second->pulse();
            }
        }
    }
}

template<class protocol_t>
void reactor_t<protocol_t>::try_spawn_activities() THROWS_NOTHING {
    std::map<typename protocol_t::region_t, typename blueprint_t<protocol_t>::role_t> blueprint_roles =
        (*blueprint->get().peers.find(get_me())).second;
    typename std::map<typename protocol_t::region_t, typename blueprint_t<protocol_t>::role_t>::iterator it;
    for (it = blueprint_roles.begin(); it != blueprint_roles.end(); it++) {
        bool none_overlap = true;
        typename std::map<
            typename protocol_t::region_t,
            std::pair<typename blueprint_t<protocol_t>::role_t, cond_t *>
            >::iterator it2;
        for (it2 = current_roles.begin(); it2 != current_roles.end(); it2++) {
            if (regions_overlap((*it).first, (*it2).first)) {
                none_overlap = false;
                break;
            }
        }
        if (none_overlap) {
            cond_t *blueprint_changed_cond = new 
        }
    }
}

template<class protocol_t>
void reactor_t<protocol_t>::run_role(
        typename protocol_t::region_t region,
        typename blueprint_t<protocol_t>::role_t role,
        cond_t *blueprint_changed_cond,
        auto_drainer_t::lock_t keepalive) THROWS_NOTHING {
}

template<class protocol_t>
void reactor_t<protocol_t>::be_primary(typename protocol_t::region_t region, store_view_t<protocol_t> *store, signal_t *blueprint_outdated, signal_t *interruptor) {
    try {
        wait_any_t interruptor_2(blueprint_outdated, interruptor);

        {
            directory_entry_t directory_entry(PRIMARY_SOON);
            /* block until all peers have acked `directory_entry` */

            /* Block until foreach key in region: foreach peer in blueprint
            scope: peer has SECONDAY_LOST, NOTHING_SOON, LISTENER, or NOTHING
            for this region */

            /* foreach key in region: backfill from most up-to-date
            SECONDARY_LOST or NOTHING_SOON */
        }

        {
            broadcaster_t<protocol_t> broadcaster(mailbox_manager, branch_history, store, &interruptor_2);

            directory_entry_t directory_entry(PRIMARY, broadcaster.get_business_card(), replier.get_business_card());

            listener_t<protocol_t> listener(mailbox_manager, branch_history, directory_entry.get_view(), &broadcaster, &interruptor_2);
            replier_t<protocol_t> replier(&listener);
            master_t<protocol_t> master(mailbox_manager, ..., &broadcaster);

            wait_interruptible(blueprint_outdated, interruptor);
        }
    } catch (interrupted_exc_t) {
        /* ignore */
    }
}

template<class protocol_t>
void reactor_t<protocol_t>::be_secondary(typename protocol_t::region_t region, store_view_t<protocol_t> *store, signal_t *blueprint_outdated, signal_t *interruptor) {
    try {
        wait_any_t interruptor_2(blueprint_outdated, interruptor);

        while (true) {
            boost::optional<clone_ptr_t<directory_single_rview_t<boost::optional<broadcaster_business_card_t<protocol_t> > > > > broadcaster =
                find_broadcaster_in_directory(region);
            if (!broadcaster) {
                /* TODO: Backfill from most up-to-date peer? */
                backfiller_t<protocol_t> backfiller(mailbox_manager, branch_history, store);
                directory_entry_t directory_entry(SECONDARY_LOST, backfiller.get_business_card(), /* current version of store */);
                broadcaster = wait_for_broadcaster_to_appear_in_directory(&interruptor_2);
            }
            try {
                listener_t<protocol_t> listener(mailbox_manager, branch_history, broadcaster, store, interruptor);
                replier_t<protocol_t> replier(&listener);
                directory_entry_t directory_entry(SECONDARY, replier.get_business_card());
                wait_interruptible(listener.get_outdated_signal(), &interruptor_2);
            } catch (typename listener_t<protocol_t>::backfiller_lost_exc_t) {
                ...
            } catch (typename listener_t<protocol_t>::data_gap_exc_t) {
                ...
            }
        }
    } catch (interrupted_exc_t) {
        /* ignore */
    }
}

template<class protocol_t>
void reactor_t<protocol_t>::be_listener(typename protocol_t::region_t region, store_view_t<protocol_t> *store, signal_t *blueprint_outdated, signal_t *interruptor) {
    try {
        wait_any_t interruptor_2(blueprint_outdated, interruptor);
        while (true) {
            boost::scoped_ptr<directory_entry_t> directory_entry(
                new directory_entry_t(LISTENER));
            /* TODO: Backfill from most up-to-date peer if broadcaster is not
            found? */
            clone_ptr_t<directory_single_rview_t<boost::optional<broadcaster_business_card_t<protocol_t> > > > broadcaster =
                wait_for_broadcaster_to_appear_in_directory(&interruptor_2);
            listener_t<protocol_t> listener(mailbox_manager, branch_history, broadcaster, store, &interruptor_2);
            directory_entry.reset(new directory_entry_t(LISTENER_READY));
        }
    } catch (interrupted_exc_t) {
        /* ignore */
    }
}

template<class protocol_t>
void reactor_t<protocol_t>::be_nothing(typename protocol_t::region_t region, store_view_t<protocol_t> *store, signal_t *blueprint_outdated, signal_t *interruptor) {
    try {
        wait_any_t interruptor_2(blueprint_outdated, interruptor);

        {
            backfiller_t<protocol_t> backfiller(mailbox_manager, branch_history, store);
            directory_entry_t directory_entry(NOTHING_SOON, backfiller.get_business_card(), /* current version of store */);
            wait_interruptible(directory_entry.get_ack_signal(), &interruptor_2);
            /* block until for each key: everything listed as a primary or secondary
            in the blueprint is listed as PRIMARY, SECONDARY, SECONDARY_LOST, or
            PRIMARY_SOON in the directory */
        }

        {
            boost::shared_ptr<typename store_view_t<protocol_t>::write_transaction_t> txn =
                store_view.begin_write_transaction(&interruptor_2);
            txn->set_metadata(region_map_t<protocol_t>(region, version_range_t::zero()));
            /* TODO make this interruptible */
            txn->reset_data();
        }

        directory_entry_t directory_entry(NOTHING);
        interruptor_2.wait_lazily_unordered();
    } catch (interrupted_exc_t) {
        /* ignore */
    }
}

