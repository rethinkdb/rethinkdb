template<class protocol_t>
dispatcher_t(
        mailbox_manager_t *mm,
        clone_ptr_t<directory_readwrite_view_t<std::map<peer_id_t, manager_state_t<protocol_t> > > > md,
        boost::shared_ptr<semilattice_read_view_t<branch_history_t<protocol_t> > > bh,
        watchable_t<manager_orders_t<protocol_t> > *o,
        std::string file_path) THROWS_NOTHING :
    mailbox_manager(mm), manager_directory(md), branch_history(bh), orders(o),
    store_file(file_path),
    backfiller(mailbox_manager, branch_history, &store_file),
    orders_subscription(boost::bind(&dispatcher_t::pump, this))
{
    watchable_freeze_t freeze(orders);
    pump();
    orders_subscription.reset(orders, &freeze);
}

template<class protocol_t>
dispatcher_t<protocol_t>::activity_type_monitor_t::activity_type_monitor_t(dispatcher_t *p, typename protocol_t::region_t r, activity_type_t e) :
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
void dispatcher_t<protocol_t>::activity_type_monitor_t::on_change() {
    if (is_failed() && !is_pulsed()) {
        pulse();
    }
}

template<class protocol_t>
bool dispatcher_t<protocol_t>::activity_type_monitor_t::is_failed() {
    std::map<typename protocol_t::region_t, typename blueprint_t<protocol_t>::shard_t> shards =
        blueprint->get().shards;
    std::map<typename protocol_t::region_t, typename blueprint_t<protocol_t>::shard_t>::iterator it = shards.find(region);
    if (it == shards.end()) {
        return true;
    }
    if (parent->get_blueprint_shard_activity_type((*it).second) != expected) {
        return true;
    }
    return false;
}

template<class protocol_t>
void dispatcher_t<protocol_t>::pump() THROWS_NOTHING {
    mutex_assertion_t::acq_t dont_let_activities_finish;
    std::map<typename protocol_t::region_t, typename blueprint_t<protocol_t>::shard_t> shards =
        blueprint->get().shards;
    for (typename std::map<typename protocol_t::region_t, typename blueprint_t<protocol_t>::shard_t>::iterator it = shards.begin();
            it != shards.end(); it++) {
        typename protocol_t::region_t region = (*it).first;
        if (!region_is_active(region)) {
            switch (get_blueprint_shard_activity_type((*it).second)) {
                case activity_type_nothing:
                    spawn_activity(
                        (*it).first,
                        boost::bind(&dispatcher_t::activity_nothing, (*it).first, _1),
                        &dont_let_activities_finish);
                    break;
                case activity_type_primary:
                    spawn_activity(
                        (*it).first,
                        boost::bind(&dispatcher_t::activity_primary, (*it).first, _1),
                        &dont_let_activities_finish);
                    break;
                case activity_type_secondary:
                    spawn_activity(
                        (*it).first,
                        boost::bind(&dispatcher_t::activity_secondary, (*it).first, _1),
                        &dont_let_activities_finish);
                    break;
            }
        }
    }
}

template<class protocol_t>
void dispatcher_t<protocol_t>::spawn_activity(typename protocol_t::region_t region,
        const boost::function<void(auto_drainer_t::lock_t)> &activity,
        mutex_assertion_t::acq_t *proof) THROWS_NOTHING {
    proof->assert_is_holding(&lock);
    rassert(!region_is_active(region));
    active_regions.insert(region);
    coro_t::spawn_sometime(boost::bind(&dispatcher_t<protocol_t>::do_activity,
        region,
        activity,
        auto_drainer_t::lock_t(&drainer)
        ));
}

template<class protocol_t>
void dispatcher_t<protocol_t>::do_activity(typename protocol_t::region_t region,
        const boost::function<void(auto_drainer_t::lock_t)> &activity,
        auto_drainer_t::lock_t keepalive) THROWS_NOTHING {
    activity(keepalive);
    {
        mutex_assertion_t::acq_t acq(&lock);
        active_regions.erase(region);
    }
    pump();
}

template<class protocol_t>
bool dispatcher_t<protocol_t>::region_is_active(typename protocol_t::region_t r) THROWS_NOTHING {
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
void dispatcher_t<protocol_t>::activity_nothing(typename protocol_t::region_t region, auto_drainer_t::lock_t keepalive) THROWS_NOTHING {
    try {
        activity_type_monitor_t blueprint_watcher(this, region, activity_type_nothing);
        wait_any_t interruptor(&blueprint_watcher, keepalive.get_drain_signal());

        /* May throw `interrupted_exc_t` */
        region_map_t<protocol_t, version_range_t> state =
            subview->begin_read_transaction(&interruptor)->get_metadata(&interruptor);

        if (state.get_as_pairs().size() != 0 || state.get_as_pairs()[0].second != version_range_t::zero()) {
            /* We have some data; we want to get rid of it as soon as it is safe
            to do so. It will be safe once the blueprint is implemented. Until
            then, make the data available to backfillees. */
            store_subview_t<protocol_t> subview(&store_file, region);
            {
                backfiller_t<protocol_t> backfiller(mailbox_manager, branch_history, &subview);
                change_shard_state_t advertisement(this, region,
                    manager_state_t<protocol_t>::cold_shard_t(state, backfiller.get_business_card()));

                /* May throw `interrupted_exc_t` */
                block_until_blueprint_implemented(&interruptor);
            }

            /* Now it's safe to destroy the data */
            /* May throw `interrupted_exc_t` */
            boost::shared_ptr<store_view_t<protocol_t>::write_transaction_t> txn =
                subview.begin_write_transaction(&interruptor);
            /* May throw `interrupted_exc_t` */
            txn->set_metadata(region_map_t<protocol_t, version_range_t>(region, version_range_t::zero()), &interruptor);
            /* May throw `interrupted_exc_t` */
            txn->reset_region(region, &interruptor);
        }

        interruptor.wait_lazily_unordered();

    } catch (interrupted_exc_t) {
        return;
    }
}

template<class protocol_t>
void dispatcher_t<protocol_t>::activity_primary(typename protocol_t::region_t region, auto_drainer_t::lock_t keepalive) THROWS_NOTHING {
}

template<class protocol_t>
void dispatcher_t<protocol_t>::activity_secondary(typename protocol_t::region_t region, auto_drainer_t::lock_t keepalive) THROWS_NOTHING {
}

