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
    blueprint_subscription(boost::bind(&reactor_t<protocol_t>::on_new_blueprint, this))
{
    {
        watchable_freeze_t freeze(blueprint);
        on_new_blueprint();
        blueprint_subscription.reset(blueprint, &freeze);
    }
}

template<class protocol_t>
void reactor_t<protocol_t>::on_new_blueprint() {

    blueprint_t<protocol_t> bp = blueprint->get();

    /* Interrupt any no-longer-current shards */
    typename std::map<typename protocol_t::region_t, std::pair<typename blueprint_t<protocol_t>::role_t, boost::shared_ptr<cond_t> > >::iterator it;
    for (it = current_activities.begin(); it != current_activities.end(); ) {
        boost::optional<typename blueprint_t<protocol_t>::role_t> new_role =
            bp->get_role((*it).first, get_me());
        if (!new_role || new_role.get() != (*it).second.first) {
            (*it).second.second->pulse();
            current_activities.remove(it++);
        } else {
            it++;
        }
    }

    /* Spawn any new shards that are necessary */
    typename std::map<typename protocol_t::region_t, typename blueprint_t<protocol_t>::role_t>::iterator it2;
    for (it2 = bp.shards.begin(); it2 != bp.shards.end(); it2++) {
        it = current_activities.find((*it2).first);
        typename blueprint_t<protocol_t>::role_t role = bp->get_role((*it2).first, get_me());
        if (it == current_activities.end()) {
            boost::shared_ptr<cond_t> blueprint_changed = boost::make_shared<cond_t>();
            current_activities.insert((*it2).first, std::make_pair(role, blueprint_changed));
            switch (role) {
                case blueprint_t<protocol_t>::role_primary:
                    coro_t::spawn_sometime(boost::bind(
                        &reactor_t<protocol_t>::be_primary, this,
                        (*it2).first, blueprint_changed, auto_drainer_t::lock_t(&drainer)
                        ));
                    break;
                case blueprint_t<protocol_t>::role_secondary:
                    coro_t::spawn_sometime(boost::bind(
                        &reactor_t<protocol_t>::be_secondary, this,
                        (*it2).first, blueprint_changed, auto_drainer_t::lock_t(&drainer)
                        ));
                    break;
                case blueprint_t<protocol_t>::role_nothing:
                    coro_t::spawn_sometime(boost::bind(
                        &reactor_t<protocol_t>::be_nothing, this,
                        (*it2).first, blueprint_changed, auto_drainer_t::lock_t(&drainer)
                        ));
                    break;
            }
        } else {
            rassert((*it).second.first == role);
        }
    }
}

template<class protocol_t>
void reactor_t<protocol_t>::be_primary(typename protocol_t::region_t region, boost::shared_ptr<cond_t> blueprint_changed, auto_drainer_t::lock_t keepalive) THROWS_NOTHING {
    try {
        gradually_grab_region_t grab(region);

        wait_any_t interruptor(keepalive.get_drain_signal(), blueprint_changed.get());
        boost::scoped_ptr<get_up_to_date_t> get_me_up_to_date(new get_up_to_date_t(this, region, &grab, &interruptor));

        /* we're up to date now */

        directory_entry_t directory_entry(region, WAITING_TO_BE_PRIMARY);

        directory_ack_waiter_t ack_waiter(directory_entry.get_timestamp());
        wait_interruptible(&ack_waiter, &interruptor);

        some_signal_subclass_t wait_for(/* directory says nothing but us is PRIMARY
             or WAITING_TO_BE_PRIMARY except for us. */);
        wait_interruptible(&wait_for, &interruptor);

        wait_interruptible(&grab, &interruptor);

        get_me_up_to_date.reset();

        boost::scoped_ptr<listener_t> listener;
        broadcaster_t<protocol_t> broadcaster(..., &listener, &interruptor);
        replier_t<protocol_t> replier(listener.get());
        master_t<protocol_t> master(..., &broadcaster);

        directory_entry.update(PRIMARY, broadcaster.get_business_card());

        wait_interruptible(&blueprint_changed.get(), keepalive.get_drain_signal());

        some_signal_subclass_t wait_for(/* foreach key in region: either there is
            something WAITING_TO_BE_PRIMARY in the directory, or the blueprint says
            that there should not be a primary. */)
        wait_interruptible(&wait_for, keepalive.get_drain_signal());

    } catch (interrupted_exc_t) {
        /* ignore */
    }
}

template<class protocol_t>
void reactor_t<protocol_t>::be_secondary(typename protocol_t::region_t region, boost::shared_ptr<cond_t> blueprint_changed, auto_drainer_t::lock_t keepalive) THROWS_NOTHING {
    try {
        wait_any_t interruptor(keepalive.get_drain_signal(), blueprint_changed.get());

        gradually_grab_region_t grab(region);
        wait_interruptible(&grab, &interruptor);

        while (true) {
            boost::optional<broadcaster_business_card_t<protocol_t> > broadcaster =
                /* find primary if any in directory */
            if (broadcaster) {
                listener_t<protocol_t> listener(broadcaster.get(), ...);
                replier_t<protocol_t> replier(&listener);

                directory_entry_t directory_entry(region, SECONDARY);

                wait_any_t waiter(&blueprint_changed.get(), listener.get_outdated_signal());
                wait_interruptible(&waiter, keepalive.get_drain_signal());

                if (blueprint_changed.get().is_pulsed()) {
                    directory_entry.update(WAITING_NOT_TO_BE_SECONDARY);

                    directory_ack_waiter_t ack_waiter(directory_entry.get_timestamp());
                    wait_interruptible(&ack_waiter, keepalive.get_drain_signal());

                    some_signal_subclass_t wait_for(/* foreach key in region: (every
                        secondary in the blueprint is SECONDARY in the directory,
                        and every primary in the blueprint is either PRIMARY or
                        WAITING_TO_BE_PRIMARY in the directory) or (our new role is
                        role_secondary or role_primary for this key) */);
                    wait_interruptible(&wait_for, keepalive.get_drain_signal());

                    return;
                }

            } else {
                backfiller_t<protocol_t> backfiller(...);

                directory_entry_t directory_entry(region, BACKFILLER);

                some_signal_subclass_t wait_for(/* there is a primary in the
                    directory for our exact bounds */);
                wait_interruptible(&wait_for, &interruptor);
            }
        }

    } catch (interrupted_exc_t) {
        /* ignore */
    }
}

template<class protocol_t>
void reactor_t<protocol_t>::be_nothing(typename protocol_t::region_t region, boost::shared_ptr<cond_t> blueprint_changed, auto_drainer_t::lock_t keepalive) THROWS_NOTHING {
    try {
        wait_any_t interruptor(keepalive.get_drain_signal(), blueprint_changed.get());

        gradually_grab_region_t grab(region);
        wait_interruptible(&grab, &interruptor);

        /* maybe wait for something */;

        {
            boost::shared_ptr<typename store_view_t<protocol_t>::write_transaction_t> txn =
                store_view.begin_write_transaction(&interruptor);
            txn->set_metadata(region_map_t<protocol_t>(region, version_range_t::zero()));
            /* TODO make this interruptible */
            txn->reset_data();
        }

        interruptor.wait_lazily_unordered();

    } catch (interrupted_exc_t) {
        /* ignore */
    }
}

