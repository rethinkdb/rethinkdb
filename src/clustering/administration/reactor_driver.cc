// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/reactor_driver.hpp"

#include <map>
#include <set>
#include <utility>

#include "errors.hpp"
#include <boost/bind.hpp>

#include "clustering/administration/machine_id_to_peer_id.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/perfmon_collection_repo.hpp"
#include "clustering/immediate_consistency/branch/multistore.hpp"
#include "clustering/reactor/blueprint.hpp"
#include "clustering/reactor/reactor.hpp"
#include "concurrency/cross_thread_watchable.hpp"
#include "concurrency/watchable.hpp"
#include "containers/incremental_lenses.hpp"
#include "rdb_protocol/store.hpp"
#include "rpc/semilattice/view/field.hpp"
#include "rpc/semilattice/watchable.hpp"
#include "stl_utils.hpp"
#include "utils.hpp"

/* This files contains the class reactor driver whose job is to create and
 * destroy reactors based on blueprints given to the server. */

stores_lifetimer_t::stores_lifetimer_t() { }

stores_lifetimer_t::~stores_lifetimer_t() {
    if (stores_.has()) {
        for (int i = 0, e = stores_.size(); i < e; ++i) {
            // TODO: This should use pmap.
            on_thread_t th(stores_[i]->home_thread());
            stores_[i].reset();
        }
    }
    if (serializer_.has()) {
        on_thread_t th(serializer_->home_thread());
        serializer_.reset();
        if (multiplexer_.has()) {
            multiplexer_.reset();
        }
    }
}


/* The reactor driver is also responsible for the translation from
`persistable_blueprint_t` to `blueprint_t`. */
blueprint_t translate_blueprint(const persistable_blueprint_t &input, const std::map<peer_id_t, machine_id_t> &translation_table) {

    blueprint_t output;
    for (persistable_blueprint_t::role_map_t::const_iterator it = input.machines_roles.begin();
            it != input.machines_roles.end(); it++) {
        peer_id_t peer = machine_id_to_peer_id(it->first, translation_table);
        if (peer.is_nil()) {
            /* We can't determine the peer ID that belongs or belonged to this
            peer because we can't reach the peer. So we generate a new peer ID
            for the peer. This works because either way, the `reactor_t` will
            be unable to reach the peer. */
            peer = peer_id_t(generate_uuid());
        }
        output.peers_roles[peer] = it->second;
    }
    return output;
}

class per_thread_ack_info_t {
public:
    per_thread_ack_info_t(const clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, machine_id_t> > > &machine_id_translation_table,
                          const semilattice_watchable_t<machines_semilattice_metadata_t> &machines_view,
                          const semilattice_watchable_t<cow_ptr_t<namespaces_semilattice_metadata_t> > &namespaces_view,
                          threadnum_t dest_thread)
        : machine_id_translation_table_(machine_id_translation_table, dest_thread),
          machines_view_(clone_ptr_t<watchable_t<machines_semilattice_metadata_t> >(machines_view.clone()), dest_thread),
          namespaces_view_(clone_ptr_t<watchable_t<cow_ptr_t<namespaces_semilattice_metadata_t> > >(namespaces_view.clone()), dest_thread) { }

    // TODO: Just get the value directly.
    // ^^^^^^^^^ What does this mean? (~daniel)
    std::map<peer_id_t, machine_id_t> get_machine_id_translation_table() {
        return machine_id_translation_table_.get_watchable()->get().get_inner();
    }

    machines_semilattice_metadata_t get_machines_view() {
        return machines_view_.get_watchable()->get();
    }

    cow_ptr_t<namespaces_semilattice_metadata_t> get_namespaces_view() {
        return namespaces_view_.get_watchable()->get();
    }

private:
    cross_thread_watchable_variable_t<change_tracking_map_t<peer_id_t, machine_id_t> > machine_id_translation_table_;
    cross_thread_watchable_variable_t<machines_semilattice_metadata_t> machines_view_;
    cross_thread_watchable_variable_t<cow_ptr_t<namespaces_semilattice_metadata_t> > namespaces_view_;
    DISABLE_COPYING(per_thread_ack_info_t);
};

class ack_info_t : public home_thread_mixin_t {
public:
    ack_info_t(const clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, machine_id_t> > > &machine_id_translation_table,
               const boost::shared_ptr<semilattice_read_view_t<machines_semilattice_metadata_t> > &machines_view,
               const boost::shared_ptr<semilattice_read_view_t<cow_ptr_t<namespaces_semilattice_metadata_t> > > &namespaces_view)
        : machine_id_translation_table_(machine_id_translation_table),
          machines_view_(machines_view),
          namespaces_view_(namespaces_view),
          per_thread_info_(get_num_db_threads()) {
        for (size_t i = 0; i < per_thread_info_.size(); ++i) {
            per_thread_info_[i].init(new per_thread_ack_info_t(machine_id_translation_table_, machines_view_, namespaces_view_, threadnum_t(i)));
        }
    }

    per_thread_ack_info_t *per_thread_ack_info() {
        return per_thread_info_[get_thread_id().threadnum].get();
    }

private:
    clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, machine_id_t> > > machine_id_translation_table_;
    semilattice_watchable_t<machines_semilattice_metadata_t> machines_view_;
    semilattice_watchable_t<cow_ptr_t<namespaces_semilattice_metadata_t> > namespaces_view_;

    scoped_array_t<scoped_ptr_t<per_thread_ack_info_t> > per_thread_info_;

    DISABLE_COPYING(ack_info_t);
};

/* This is in part because these types aren't copyable so they can't go in
 * a std::pair. This class is used to hold a reactor and a watchable that
 * it's watching. */
class watchable_and_reactor_t : private ack_checker_t {
public:
    watchable_and_reactor_t(const base_path_t &_base_path,
                            io_backender_t *io_backender,
                            reactor_driver_t *parent,
                            namespace_id_t namespace_id,
                            const blueprint_t &bp,
                            svs_by_namespace_t *svs_by_namespace,
                            rdb_context_t *_ctx) :
        base_path(_base_path),
        watchable(bp),
        ctx(_ctx),
        parent_(parent),
        namespace_id_(namespace_id),
        svs_by_namespace_(svs_by_namespace)
    {
        coro_t::spawn_sometime(boost::bind(&watchable_and_reactor_t::initialize_reactor, this, io_backender));
    }

    ~watchable_and_reactor_t() {
        /* Make sure that the coro we spawn to initialize this things has
         * actually run. */
        reactor_has_been_initialized_.wait_lazily_unordered();

        /* XXX the order in which the perform the operations is important and
         * will cause bugs if any changes are made. */

        /* First we destroy the subscription, this is because anytime this
         * subscription receives a notification it propagates it to the reactor
         * which we are about to destroy. If this line were after the reactory
         * destruction we would get segfaults in on_change. */
        reactor_directory_subscription_.reset();

        /* Destroy the reactor. (Dun dun duhnnnn...). Next we destroy the
         * reactor. We need to do this before we remove the reactor bcard. This
         * is because there exists parts of the be_[role] (be_primary,
         * be_secondary etc.) function which assume that the reactors own bcard
         * will be in place for their duration. */
        reactor_.reset();

        /* Finally we remove the reactor bcard. */
        parent_->set_reactor_directory_entry(namespace_id_,
            boost::optional<reactor_driver_t::reactor_directory_entry_t>());
    }

    static bool compute_is_acceptable_ack_set(const std::set<peer_id_t> &acks, const namespace_id_t &namespace_id, per_thread_ack_info_t *ack_info) {
        /* There are a bunch of weird corner cases: what if the namespace was
        deleted? What if we got an ack from a machine but then it was declared
        dead? What if the namespaces `expected_acks` field is in conflict? We
        handle the weird cases by erring on the side of reporting that there
        are not enough acks yet. If a machine's `expected_acks` field is in
        conflict, for example, then all writes will report that there are not
        enough acks. That's a bit weird, but fortunately it can't lead to data
        corruption. */
        std::multiset<datacenter_id_t> acks_by_dc;
        std::map<peer_id_t, machine_id_t> translation_table_snapshot = ack_info->get_machine_id_translation_table();
        machines_semilattice_metadata_t mmd = ack_info->get_machines_view();
        cow_ptr_t<namespaces_semilattice_metadata_t> nmd = ack_info->get_namespaces_view();

        for (std::set<peer_id_t>::const_iterator it = acks.begin(); it != acks.end(); it++) {
            std::map<peer_id_t, machine_id_t>::iterator tt_it = translation_table_snapshot.find(*it);
            if (tt_it == translation_table_snapshot.end()) continue;
            machines_semilattice_metadata_t::machine_map_t::iterator jt = mmd.machines.find(tt_it->second);
            if (jt == mmd.machines.end()) continue;
            if (jt->second.is_deleted()) continue;
            if (jt->second.get_ref().datacenter.in_conflict()) continue;
            datacenter_id_t dc = jt->second.get_ref().datacenter.get();
            acks_by_dc.insert(dc);
        }
        namespaces_semilattice_metadata_t::namespace_map_t::const_iterator it =
            nmd->namespaces.find(namespace_id);
        if (it == nmd->namespaces.end()) return false;
        if (it->second.is_deleted()) return false;
        if (it->second.get_ref().ack_expectations.in_conflict()) return false;
        std::map<datacenter_id_t, ack_expectation_t> expected_acks = it->second.get_ref().ack_expectations.get();

        /* The nil uuid represents acks from anywhere. */
        uint32_t extra_acks = 0;
        for (std::map<datacenter_id_t, ack_expectation_t>::const_iterator kt = expected_acks.begin(); kt != expected_acks.end(); ++kt) {
            if (!kt->first.is_nil()) {
                if (acks_by_dc.count(kt->first) < kt->second.expectation()) {
                    return false;
                }
                extra_acks += acks_by_dc.count(kt->first) - kt->second.expectation();
            }
        }

        /* Add in the acks that came from datacenters we had no expectations
         * for (or the nil datacenter). */
        for (std::multiset<datacenter_id_t>::iterator at  = acks_by_dc.begin();
                                                      at != acks_by_dc.end();
                                                      ++at) {
            if (!std_contains(expected_acks, *at) || at->is_nil()) {
                extra_acks++;
            }
        }

        if (extra_acks < expected_acks[nil_uuid()].expectation()) {
            return false;
        }
        return true;
    }

    bool is_acceptable_ack_set(const std::set<peer_id_t> &acks) const {
        return compute_is_acceptable_ack_set(acks, namespace_id_, parent_->ack_info->per_thread_ack_info());
    }

    // Computes what write durability we should use when sending a write to the
    // given peer, on the given table.  Uses the ack_info parameter to figure
    // out the answer to this question.  Figures out the answer much like how
    // compute_is_acceptable_ack_set figures out whether it has the right ack
    // set.  It figures out what datacenter the peer belongs to, and then, using
    // the ack_expectations_t for the given namespace, figures out whether we
    // want hard durability or soft durability for that datacenter.
    static write_durability_t compute_write_durability(const peer_id_t &peer, const namespace_id_t &namespace_id, per_thread_ack_info_t *ack_info) {
        // FML
        const std::map<peer_id_t, machine_id_t> translation_table_snapshot = ack_info->get_machine_id_translation_table();
        auto it = translation_table_snapshot.find(peer);
        if (it == translation_table_snapshot.end()) {
            // What should we do?  I have no idea.  Default to HARD, let somebody else handle
            // the peer not existing.
            return write_durability_t::HARD;
        }

        const machine_id_t machine_id = it->second;

        machines_semilattice_metadata_t mmd = ack_info->get_machines_view();
        std::map<machine_id_t, deletable_t<machine_semilattice_metadata_t> >::const_iterator machine_map_it
            = mmd.machines.find(machine_id);
        if (machine_map_it == mmd.machines.end() || machine_map_it->second.is_deleted()
            || machine_map_it->second.get_ref().datacenter.in_conflict()) {
            // Is there something smart to do?  Besides deleting this whole class and
            // refactoring clustering not to do O(n^2) work per request?  Default to HARD, let
            // somebody else handle the machine not existing.
            return write_durability_t::HARD;
        }

        const datacenter_id_t dc = machine_map_it->second.get_ref().datacenter.get();

        cow_ptr_t<namespaces_semilattice_metadata_t> nmd = ack_info->get_namespaces_view();

        std::map<namespace_id_t, deletable_t<namespace_semilattice_metadata_t> >::const_iterator ns_it
            = nmd->namespaces.find(namespace_id);

        if (ns_it == nmd->namespaces.end() || ns_it->second.is_deleted() || ns_it->second.get_ref().ack_expectations.in_conflict()) {
            // Again, FML, we default to HARD.
            return write_durability_t::HARD;
        }

        std::map<datacenter_id_t, ack_expectation_t> ack_expectations = ns_it->second.get_ref().ack_expectations.get();
        auto ack_it = ack_expectations.find(dc);
        if (ack_it == ack_expectations.end()) {
            // Yet again, FML, we default to HARD.
            return write_durability_t::HARD;
        }

        return ack_it->second.is_hardly_durable() ? write_durability_t::HARD : write_durability_t::SOFT;
    }

    write_durability_t get_write_durability(const peer_id_t &peer) const {
        return compute_write_durability(peer, namespace_id_, parent_->ack_info->per_thread_ack_info());
    }

private:
    typedef boost::optional<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t> > >
        extract_reactor_directory_per_peer_result_type;

    extract_reactor_directory_per_peer_result_type extract_reactor_directory_per_peer(
        const namespaces_directory_metadata_t &nss) {

        auto it = nss.reactor_bcards.find(namespace_id_);
        if (it == nss.reactor_bcards.end()) {
            return boost::optional<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t> > >();
        } else {
            return boost::optional<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t> > >(it->second);
        }
    }

    void on_change_reactor_directory() {
        // Tell our parent, the reactor driver, that this reactor's directory
        // has changed, and what its new value is:
        parent_->set_reactor_directory_entry(namespace_id_,
            boost::make_optional(reactor_->get_reactor_directory()->get()));
    }

    void initialize_reactor(io_backender_t *io_backender) {
        perfmon_collection_repo_t::collections_t *perfmon_collections = parent_->perfmon_collection_repo->get_perfmon_collections_for_namespace(namespace_id_);
        perfmon_collection_t *namespace_collection = &perfmon_collections->namespace_collection;
        perfmon_collection_t *serializers_collection = &perfmon_collections->serializers_collection;

        // TODO: We probably shouldn't have to pass in this perfmon collection.
        svs_by_namespace_->get_svs(serializers_collection, namespace_id_, &stores_lifetimer_, &svs_, ctx);

        auto const extract_reactor_directory_per_peer_fun =
            boost::bind(&watchable_and_reactor_t::extract_reactor_directory_per_peer,
                        this, _1);
        incremental_map_lens_t<peer_id_t,
                               namespaces_directory_metadata_t,
                               typeof extract_reactor_directory_per_peer_fun>
            extract_reactor_directory(extract_reactor_directory_per_peer_fun);

        reactor_.init(new reactor_t(
            base_path,
            io_backender,
            parent_->mbox_manager,
            &parent_->backfill_throttler,
            this,
            parent_->directory_view->incremental_subview(extract_reactor_directory),
            parent_->branch_history_manager,
            watchable.get_watchable(),
            svs_.get(), namespace_collection, ctx));

        {
            watchable_t<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t> > >::freeze_t reactor_directory_freeze(reactor_->get_reactor_directory());
            reactor_directory_subscription_.init(
                new watchable_t<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t> > >::subscription_t(
                    boost::bind(&watchable_and_reactor_t::on_change_reactor_directory, this),
                    reactor_->get_reactor_directory(), &reactor_directory_freeze));

            parent_->set_reactor_directory_entry(namespace_id_,
                boost::make_optional(reactor_->get_reactor_directory()->get()));
        }

        reactor_has_been_initialized_.pulse();
    }

private:
    const base_path_t base_path;
public:
    watchable_variable_t<blueprint_t> watchable;

    rdb_context_t *const ctx;

private:
    cond_t reactor_has_been_initialized_;

    reactor_driver_t *const parent_;
    const namespace_id_t namespace_id_;
    svs_by_namespace_t *const svs_by_namespace_;

    stores_lifetimer_t stores_lifetimer_;
    scoped_ptr_t<multistore_ptr_t> svs_;
    scoped_ptr_t<reactor_t> reactor_;

    scoped_ptr_t<watchable_t<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t> > >::subscription_t> reactor_directory_subscription_;

    DISABLE_COPYING(watchable_and_reactor_t);
};

reactor_driver_t::reactor_driver_t(const base_path_t &_base_path,
                                   io_backender_t *_io_backender,
                                   mailbox_manager_t *_mbox_manager,
                                   const clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, namespaces_directory_metadata_t> > > &_directory_view,
                                   branch_history_manager_t *_branch_history_manager,
                                   boost::shared_ptr<semilattice_readwrite_view_t<cow_ptr_t<namespaces_semilattice_metadata_t> > > _namespaces_view,
                                   boost::shared_ptr<semilattice_read_view_t<machines_semilattice_metadata_t> > machines_view_,
                                   const clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, machine_id_t> > > &_machine_id_translation_table,
                                   svs_by_namespace_t *_svs_by_namespace,
                                   perfmon_collection_repo_t *_perfmon_collection_repo,
                                   rdb_context_t *_ctx)
    : base_path(_base_path),
      io_backender(_io_backender),
      mbox_manager(_mbox_manager),
      directory_view(_directory_view),
      branch_history_manager(_branch_history_manager),
      machine_id_translation_table(_machine_id_translation_table),
      namespaces_view(_namespaces_view),
      machines_view(machines_view_),
      ctx(_ctx),
      svs_by_namespace(_svs_by_namespace),
      ack_info(new ack_info_t(machine_id_translation_table, machines_view, namespaces_view)),
      watchable_variable(namespaces_directory_metadata_t()),
      semilattice_subscription(boost::bind(&reactor_driver_t::on_change, this), namespaces_view),
      translation_table_subscription(boost::bind(&reactor_driver_t::on_change, this)),
      perfmon_collection_repo(_perfmon_collection_repo)
{
    watchable_t<change_tracking_map_t<peer_id_t, machine_id_t> >::freeze_t freeze(machine_id_translation_table);
    translation_table_subscription.reset(machine_id_translation_table, &freeze);
    on_change();
}

reactor_driver_t::~reactor_driver_t() {
    /* This must be defined in the `.tcc` file because the full definition of
    `watchable_and_reactor_t` is not available in the `.hpp` file. */
}

void reactor_driver_t::set_reactor_directory_entry(
    const namespace_id_t reactor_namespace,
    const boost::optional<reactor_directory_entry_t> &new_value) {

    // Just stash the change for now. If necessary, we spawn a coroutine that takes
    // care of eventually committing the changes to `watchable_variable`.
    // This allows us to combine multiple reactor directory changes into a single
    // update of the watchable_variable, which significantly reduces the overhead
    // of metadata operations that affect multiple reactors (such as resharding)
    // in large clusters.
    const bool must_initiate_commit = changed_reactor_directories.empty();
    changed_reactor_directories[reactor_namespace] = new_value;

    if (must_initiate_commit) {
        coro_t::spawn_sometime(
            boost::bind(&reactor_driver_t::commit_directory_changes,
                        this,
                        auto_drainer_t::lock_t(&directory_change_drainer)));
    }
}

void reactor_driver_t::commit_directory_changes(auto_drainer_t::lock_t lock) {
    // Delay the commit for a moment in anticipation that more changes come in
    lock.assert_is_holding(&directory_change_drainer);
    try {
        // The nap time of 200 ms was determined experimentally as follows:
        // In the specific test, a 200 ms nap provided a high speed up when
        // resharding a table in a cluster of 64 nodes. Higher values improved the
        // efficiency of the operation only marginally, while unnecessarily slowing
        // down directory changes in smaller clusters.
        nap(200, lock.get_drain_signal());
    } catch (const interrupted_exc_t &e) {
    }
    lock.assert_is_holding(&directory_change_drainer);

    DEBUG_VAR mutex_assertion_t::acq_t acq(&watchable_variable_lock);

    watchable_variable.apply_atomic_op(std::bind(&apply_directory_changes,
                                                 &changed_reactor_directories,
                                                 std::placeholders::_1));
}

bool reactor_driver_t::apply_directory_changes(
    std::map<namespace_id_t, boost::optional<reactor_directory_entry_t> >
        *_changed_reactor_directories,
    namespaces_directory_metadata_t *directory) {

    for (auto it = _changed_reactor_directories->begin();
         it != _changed_reactor_directories->end();
         ++it) {
        // it->second is a boost::optional<reactor_directory_entry_t>
        if (it->second) {
            directory->reactor_bcards[it->first] = it->second.get();
        } else {
            directory->reactor_bcards.erase(it->first);
        }
    }
    _changed_reactor_directories->clear();
    return true;
}

void reactor_driver_t::delete_reactor_data(
        auto_drainer_t::lock_t lock,
        watchable_and_reactor_t *thing_to_delete,
        namespace_id_t namespace_id)
{
    lock.assert_is_holding(&drainer);
    delete thing_to_delete;
    svs_by_namespace->destroy_svs(namespace_id);
}

void reactor_driver_t::on_change() {
    cow_ptr_t<namespaces_semilattice_metadata_t> namespaces = namespaces_view->get();
    std::map<peer_id_t, machine_id_t> machine_id_translation_table_value = machine_id_translation_table->get().get_inner();

    for (namespaces_semilattice_metadata_t::namespace_map_t::const_iterator
             it = namespaces->namespaces.begin(); it != namespaces->namespaces.end(); it++) {
        if (it->second.is_deleted() && std_contains(reactor_data, it->first)) {
            /* on_change cannot block because it is called as part of
             * semilattice subscription, however the
             * watchable_and_reactor_t destructor can block... therefore
             * bullshit takes place. We must release a value from the
             * ptr_map into this bullshit auto_type so that it's not in the
             * map but the destructor hasn't been called... then this needs
             * to be heap allocated so that it can be safely passed to a
             * coroutine for destruction. */
            watchable_and_reactor_t *reactor_datum = reactor_data.at(it->first).release();
            reactor_data.erase(it->first);
            coro_t::spawn_sometime(boost::bind(
                &reactor_driver_t::delete_reactor_data,
                this,
                auto_drainer_t::lock_t(&drainer),
                reactor_datum,
                it->first));
        } else if (!it->second.is_deleted()) {
            const persistable_blueprint_t *pbp = NULL;

            try {
                pbp = &it->second.get_ref().blueprint.get_ref();
            } catch (const in_conflict_exc_t &) {
                //Nothing to do for this namespaces, its blueprint is in
                //conflict.
                continue;
            }

            blueprint_t bp = translate_blueprint(*pbp, machine_id_translation_table_value);

            if (std_contains(bp.peers_roles, mbox_manager->get_connectivity_service()->get_me())) {
                /* Either construct a new reactor (if this is a namespace we
                 * haven't seen before). Or send the new blueprint to the
                 * existing reactor. */
                if (!std_contains(reactor_data, it->first)) {
                    namespace_id_t tmp = it->first;
                    reactor_data.insert(
                            std::make_pair(tmp,
                                           make_scoped<watchable_and_reactor_t>(base_path, io_backender, this, it->first, bp, svs_by_namespace, ctx)));
                } else {
                    struct op_closure_t {
                        static bool apply(const blueprint_t &_bp,
                                          blueprint_t *bp_ref) {
                            const bool blueprint_changed = (*bp_ref != _bp);
                            if (blueprint_changed) {
                                *bp_ref = _bp;
                            }
                            return blueprint_changed;
                        }
                    };

                    reactor_data.find(it->first)->second->watchable.apply_atomic_op(std::bind(&op_closure_t::apply, std::ref(bp), std::placeholders::_1));
                }
            } else {
                /* The blueprint does not mentions us so we destroy the
                 * reactor. */
                if (std_contains(reactor_data, it->first)) {
                    watchable_and_reactor_t *reactor_datum = reactor_data.at(it->first).release();
                    reactor_data.erase(it->first);
                    coro_t::spawn_sometime(boost::bind(&reactor_driver_t::delete_reactor_data, this, auto_drainer_t::lock_t(&drainer), reactor_datum, it->first));
                }
            }
        }
    }
}
