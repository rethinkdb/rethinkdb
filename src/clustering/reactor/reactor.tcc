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
    orders_subscription(boost::bind(&delayed_pump_t::run_eventually, &pumper)),
    directory_subscription(
        boost::bind(&delayed_pump_t::run_eventually, &pumper),
        boost::bind(&delayed_pump_t::run_eventually, &pumper),
        boost::bind(&delayed_pump_t::run_eventually, &pumper))
{
    {
        watchable_freeze_t freeze(orders);
        orders_subscription.reset(orders, &freeze);
    }
    {
        directory_read_service_t::whole_directory_freeze_t freeze(reactor_directory->get_directory_service());
        directory_subscription.reset(reactor_directory->get_directory_service(), &freeze);
    }
    pumper.run_eventually();
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
    mutex_assertion_t::acq_t dont_let_activities_finish(&lock);
    directory_read_service_t::whole_directory_freeze_t dont_let_directory_change(reactor_directory->get_directory_service());
    watchable_freeze_t dont_let_blueprint_change(orders);
    ASSERT_NO_CORO_WAITING;
    std::map<typename protocol_t::region_t, typename blueprint_t<protocol_t>::shard_t> shards =
        blueprint->get().shards;
    for (typename std::map<typename protocol_t::region_t, typename blueprint_t<protocol_t>::shard_t>::iterator it = shards.begin();
            it != shards.end(); it++) {
        typename protocol_t::region_t region = (*it).first;
        switch ((*it).second.get_role(mailbox_manager->get_connectivity_service()->get_me())) {
            case blueprint_t<protocol_t>::role_nothing: {
                if (/* every secondary in blueprint is up according to the directory */ && !region_is_active(region)) {
                    spawn_activity(region, boost::bind(&reactor_t<protocol_t>::reset_region, this, region, _1));
                } else {
                    /* for each inactive subregion: set up a backfiller for that
                    subregion */
                }
                break;
            }
            case blueprint_t<protocol_t>::role_primary: {
                if (/* our state is as far forward as anyone else according to the directory, and no other primary is up */ && !region_is_active(region)) {
                    spawn_activity(region, boost::bind(&reactor_t<protocol_t>::be_primary, this, region, _1));
                } else {
                    /* for each inactive subregion where we are lagging behind
                    another peer: start backfilling from that peer */
                }
                break;
            }
            case blueprint_t<protocol_t>::role_secondary: {
                if (/* there is a primary in the directory */ && !region_is_active(region)) {
                    spawn_activity(region, boost::bind(&reactor_t<protocol_t>::be_secondary, this, region, _1));
                } else {
                    for (/* each inactive subregion in region */) {
                        if (/* we are ahead of everything in directory */) {
                            spawn_activity(subregion, boost::bind(&reactor_t<protocol_t>::offer_backfills, this, region, _1));
                        } else if (/* there is a unique most recent backfiller */) {
                            spawn_activity(subregion, boost::bind(&backfillee<protocol_t>, 
                        }
                    }
                }
                break;
            }
        }
    }
}

template<class protocol_t>
void reactor_t<protocol_t>::spawn_activity(typename protocol_t::region_t region,
        const boost::function<void(signal_t *)> &activity,
        mutex_assertion_t *proof_of_activity_mutex) THROWS_NOTHING {
    proof_of_activity_mutex->assert_is_holding(&lock);
    rassert(!region_is_active(region));
    active_regions.insert(region);
    coro_t::spawn_sometime(boost::bind(&reactor_t<protocol_t>::run_activity,
        this, region, activity, auto_drainer_t::lock_t(&drainer)));
}

template<class protocol_t>
void reactor_t<protocol_t>::run_activity(typename protocol_t::region_t region,
        const boost::function<void(signal_t *)> &activity,
        auto_drainer_t::lock_t keepalive) THROWS_NOTHING {
    {
        store_subview_t sub_store(&store_file, region);
        activity(&sub_store, keepalive.get_drain_signal());
    }
    {
        mutex_assertion_t::acq_t acq(&lock);
        active_regions.erase(region);
    }
    pumper.run_eventually();
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
void reactor_t<protocol_t>::reset_region(typename protocol_t::region_t region, store_view_t *sub_store, signal_t *interruptor) THROWS_NOTHING {
    try {
        role_monitor_t role_monitor(this, region, blueprint_t<protocol_t>::role_nothing);
        wait_any_t interruptor2(&role_monitor, interruptor);
        {
            boost::shared_ptr<typename store_view_t<protocol_t>::write_transaction_t> txn =
                store_view.begin_write_transaction(&interruptor2);
            txn->set_metadata(region_map_t<protocol_t>(region, version_range_t::zero()));
            /* TODO make this interruptible */
            txn->reset_data();
        }
        interruptor2.wait_lazily_unordered();
    } catch (interrupted_exc_t) {
        /* Ignore */
    }
}

template<class protocol_t>
void reactor_t<protocol_t>::be_primary(typename protocol_t::region_t region, store_view_t *sub_store, signal_t *interruptor) THROWS_NOTHING {
    try {
        boost::scoped_ptr<listener_t<protocol_t> > listener;
        broadcaster_t<protocol_t> broadcaster(..., &listener, interruptor);
        replier_t<protocol_t> replier(listener.get());
        master_t<protocol_t> master(...)
    
        some_signal_subclass_t thing; /* Will be pulsed when blueprint says someone else is primary of a shard overlapping ours and directory says
            foreach primary of a shard overlapping ours: foreach subregion of their region: if there's an active
            primary, they're live-tracking it, else they're most up-to-date of any node */
        wait_any_t waiter(&thing, interruptor);
        waiter.wait_lazily_unordered();
    } catch (interrupted_exc_t) {
        /* Ignore */
    }
}

template<class protocol_t>
void reactor_t<protocol_t>::be_secondary(typename protocol_t::region_t region, store_view_t<protocol_t> *sub_store, signal_t *interruptor) THROWS_NOTHING {
    try {
        listener_t<protocol_t> listener(...);
        replier_t<protocol_t> replier(&listener);
    
        some_signal_subclass_t thing; /* Will be pulsed when blueprint says ? */
        wait_any_t waiter(&thing, listener.get_outdated_signal(), interruptor);
        waiter.wait_lazily_unordered();
    } catch (interrupted_exc_t) {
        /* Ignore */
    }
}

template<class protocol_t>
void backfill_in(typename protocol_t::region_t region, store_view_t<protocol_t> *sub_store, clone_ptr_t<directory_single_rview_t<backfiller_business_card_t<protocol_t> > > backfiller, signal_t *interruptor) THROWS_NOTHING {
    try {
        backfillee<protocol_t>(
            mailbox_manager,
            branch_history,
            sub_store,
            backfiller,
            interruptor);
    } catch (interrupted_exc_t) {
        /* Ignore */
    }
}

template<class protocol_t>
void reactor_t<protocol_t>::offer_backfills(typename protocol_t::region_t region, store_view_t<protocol_t> *sub_store, signal_t *interruptor) THROWS_NOTHING {
    try {
        backfiller_t<protocol_t> backfiller(
            mailbox_manager,
            branch_history,
            sub_store);
        some_signal_subclass_t thing; /* Will be pulsed when the conditions
            under which we started offering backfills no longer hold */
        wait_interruptible(&thing, interruptor);
    } catch (interrupted_exc_t) {
        /* Ignore */
    }
}
