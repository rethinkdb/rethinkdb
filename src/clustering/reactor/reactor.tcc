template<class protocol_t>
reactor_t<protocol_t>::reactor_t(
        mailbox_manager_t *mm,
        clone_ptr_t<directory_readwrite_view_t<reactor_business_card_t<protocol_t> > > rd,
        boost::shared_ptr<semilattice_read_view_t<branch_history_t<protocol_t> > > bh,
        watchable_t<manager_orders_t<protocol_t> > *o,
        std::string file_path) THROWS_NOTHING :
    mailbox_manager(mm), reactor_directory(rd), branch_history(bh), orders(o),
    store_file(file_path),
    backfiller(mailbox_manager, branch_history, &store_file),
    orders_subscription(boost::bind(&reactor_t::pump, this))
{
    watchable_freeze_t freeze(orders);
    pump();
    orders_subscription.reset(orders, &freeze);
}

template<class protocol_t>
reactor_t<protocol_t>::role_monitor_t::role_monitor_t(reactor_t *p, typename protocol_t::region_t r, activity_type_t e) :
    parent(p), region(r), expected(e),
    subscription(boost::bind(&activity_type_monitor_t::on_change, this))
{
    watchable_freeze_t freeze(parent->blueprint);
    if (is_failed()) {
        pulse();
    } else {
        subscription.reset(parent->blueprint)
    }
}

template<class protocol_t>
void reactor_t<protocol_t>::role_monitor_t::on_change() {
    if (is_failed() && !is_pulsed()) {
        pulse();
    }
}

template<class protocol_t>
bool reactor_t<protocol_t>::role_monitor_t::is_failed() {
    boost::optional<typename blueprint_t<protocol_t>::role_t> role =
        parent->blueprint->get().get_role(
            parent->mailbox_manager->get_connectivity_service()->get_me(),
            region
            );
    return role && role.get() == expected;
}

template<class protocol_t>
void reactor_t<protocol_t>::pump() THROWS_NOTHING {
    mutex_assertion_t::acq_t dont_let_activities_finish;
    std::map<typename protocol_t::region_t, typename blueprint_t<protocol_t>::shard_t> shards =
        blueprint->get().shards;
    for (typename std::map<typename protocol_t::region_t, typename blueprint_t<protocol_t>::shard_t>::iterator it = shards.begin();
            it != shards.end(); it++) {
        typename protocol_t::region_t region = (*it).first;
        if (!region_is_active(region)) {
            active_regions.insert(region);
            coro_t::spawn_sometime(boost::bind(&reactor_t<protocol_t>::do_role,
                this,
                region,
                (*it).second.get_role(mailbox_manager->get_connectivity_service()->get_me()),
                auto_drainer_t::lock_t(&drainer)
                ));
        }
    }
}

template<class protocol_t>
void reactor_t<protocol_t>::do_role(typename protocol_t::region_t region,
        typename blueprint_t<protocol_t>::role_t role,
        auto_drainer_t::lock_t keepalive) THROWS_NOTHING {
    role_monitor_t role_monitor(this, region, role);
    wait_any_t interruptor(&role_monitor, keepalive.get_drain_signal());
    try {
        switch (role) {
            case blueprint_t<protocol_t>::role_nothing:
                do_role_nothing(region, &interruptor);
                crash("expected do_role_nothing() to throw exception");
            case blueprint_t<protocol_t>::role_primary:
                do_role_primary(region, &interruptor);
                crash("expected do_role_primary() to throw exception");
            case blueprint_t<protocol_t>::role_secondary:
                do_role_secondary(region, &interruptor);
                crash("expected do_role_secondary() to throw exception");
        }
    } catch (interrupted_exc_t) {
        /* This is what we expected */
    }
    {
        mutex_assertion_t::acq_t acq(&lock);
        active_regions.erase(region);
    }
    pump();
}

template<class protocol_t>
bool reactor_t<protocol_t>::region_is_active(typename protocol_t::region_t r) THROWS_NOTHING {
    for (typename std::set<typename protocol_t::region_t>::iterator it = 
            regions_with_a_primary_or_secondary.begin();
            it != regions_with_a_primary_or_secondary.end(); it++) {
        if (regions_overlap(*it, r)) {
            return true;
        }
    }
    return false;
}

template<class protocol_t>
void reactor_t<protocol_t>::do_role_nothing(typename protocol_t::region_t region, store_view_t<protocol_t> *subview, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {

    /*
    Wait until ???
    Stop broadcaster, listener, replier, backfiller.
    Delete data.
    Wait.
    */







    /* May throw `interrupted_exc_t` */
    region_map_t<protocol_t, version_range_t> state =
        subview->begin_read_transaction(&interruptor)->get_metadata(interruptor);

    /* Do we have any data? */
    if (state.get_as_pairs().size() != 0 || state.get_as_pairs()[0].second != version_range_t::zero()) {
        /* We have some data; we want to get rid of it as soon as it is safe
        to do so. It will be safe once the blueprint is implemented. Until
        then, make the data available to backfillees. */
        {
            backfiller_t<protocol_t> backfiller(mailbox_manager, branch_history, &subview);
            change_shard_state_t advertisement(this, region,
                manager_state_t<protocol_t>::cold_shard_t(state, backfiller.get_business_card()));

            block_until_primaries_and_secondaries_up_for_shard(region, interruptor);  /* May throw `interrupted_exc_t` */
            check_with_peers(region, blueprint_t<protocol_t>::role_nothing, interruptor);   /* May throw `interrupted_exc_t` */
        }

        /* Now it's safe to destroy the data */
        boost::shared_ptr<store_view_t<protocol_t>::write_transaction_t> txn =
            subview.begin_write_transaction(&interruptor);   /* May throw `interrupted_exc_t` */
        txn->set_metadata(region_map_t<protocol_t, version_range_t>(region, version_range_t::zero()), &interruptor);   /* May throw `interrupted_exc_t` */
        txn->reset_region(region, &interruptor);   /* May throw `interrupted_exc_t` */
    }

    interruptor.wait_lazily_unordered();
    throw interrupted_exc_t();
}

template<class protocol_t>
void reactor_t<protocol_t>::do_role_primary(typename protocol_t::region_t region, store_view_t<protocol_t> *subview, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {

    /*
    
    Wait until there is no broadcaster for any overlapping region.
    
}

template<class protocol_t>
void reactor_t<protocol_t>::do_role_secondary(typename protocol_t::region_t region, store_view_t<protocol_t> *subview, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
}

