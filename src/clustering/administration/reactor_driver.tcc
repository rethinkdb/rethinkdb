#ifndef CLUSTERING_ADMINISTRATION_REACTOR_DRIVER_TCC_
#define CLUSTERING_ADMINISTRATION_REACTOR_DRIVER_TCC_

#include "clustering/administration/reactor_driver.hpp"

#include <map>
#include <set>

#include "clustering/administration/machine_id_to_peer_id.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/perfmon_collection_repo.hpp"
#include "clustering/immediate_consistency/branch/multistore.hpp"
#include "clustering/reactor/blueprint.hpp"
#include "clustering/reactor/reactor.hpp"
#include "concurrency/watchable.hpp"
#include "db_thread_info.hpp"
#include "rpc/semilattice/view/field.hpp"

/* This files contains the class reactor driver whose job is to create and
 * destroy reactors based on blueprints given to the server. */

/* The reactor driver is also responsible for the translation from
`persistable_blueprint_t` to `blueprint_t`. */
template<class protocol_t>
blueprint_t<protocol_t> translate_blueprint(const persistable_blueprint_t<protocol_t> &input, const std::map<peer_id_t, machine_id_t> &translation_table) {

    blueprint_t<protocol_t> output;
    for (typename persistable_blueprint_t<protocol_t>::role_map_t::const_iterator it = input.machines_roles.begin();
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

template <class protocol_t>
class per_thread_ack_info_t {
public:
    per_thread_ack_info_t(const clone_ptr_t<watchable_t<std::map<peer_id_t, machine_id_t> > > &machine_id_translation_table,
                          const semilattice_watchable_t<machines_semilattice_metadata_t> &machines_view,
                          const semilattice_watchable_t<cow_ptr_t<namespaces_semilattice_metadata_t<protocol_t> > > &namespaces_view,
                          int dest_thread)
        : machine_id_translation_table_(machine_id_translation_table, dest_thread),
          machines_view_(clone_ptr_t<watchable_t<machines_semilattice_metadata_t> >(machines_view.clone()), dest_thread),
          namespaces_view_(clone_ptr_t<watchable_t<cow_ptr_t<namespaces_semilattice_metadata_t<protocol_t> > > >(namespaces_view.clone()), dest_thread) { }

    // TODO: Just get the value directly.
    std::map<peer_id_t, machine_id_t> get_machine_id_translation_table() {
        return machine_id_translation_table_.get_watchable()->get();
    }

    machines_semilattice_metadata_t get_machines_view() {
        return machines_view_.get_watchable()->get();
    }

    cow_ptr_t<namespaces_semilattice_metadata_t<protocol_t> > get_namespaces_view() {
        return namespaces_view_.get_watchable()->get();
    }

private:
    cross_thread_watchable_variable_t<std::map<peer_id_t, machine_id_t> > machine_id_translation_table_;
    cross_thread_watchable_variable_t<machines_semilattice_metadata_t> machines_view_;
    cross_thread_watchable_variable_t<cow_ptr_t<namespaces_semilattice_metadata_t<protocol_t> > > namespaces_view_;
    DISABLE_COPYING(per_thread_ack_info_t);
};

template <class protocol_t>
class ack_info_t : public home_thread_mixin_t {
public:
    ack_info_t(const clone_ptr_t<watchable_t<std::map<peer_id_t, machine_id_t> > > &machine_id_translation_table,
               const boost::shared_ptr<semilattice_read_view_t<machines_semilattice_metadata_t> > &machines_view,
               const boost::shared_ptr<semilattice_read_view_t<cow_ptr_t<namespaces_semilattice_metadata_t<protocol_t> > > > &namespaces_view)
        : machine_id_translation_table_(machine_id_translation_table),
          machines_view_(machines_view),
          namespaces_view_(namespaces_view),
          per_thread_info_(get_num_db_threads()) {
        for (ssize_t i = 0; i < per_thread_info_.size(); ++i) {
            per_thread_info_[i].init(new per_thread_ack_info_t<protocol_t>(machine_id_translation_table_, machines_view_, namespaces_view_, i));
        }
    }

    per_thread_ack_info_t<protocol_t> *per_thread_ack_info() {
        return per_thread_info_[get_thread_id()].get();
    }

private:
    clone_ptr_t<watchable_t<std::map<peer_id_t, machine_id_t> > > machine_id_translation_table_;
    semilattice_watchable_t<machines_semilattice_metadata_t> machines_view_;
    semilattice_watchable_t<cow_ptr_t<namespaces_semilattice_metadata_t<protocol_t> > > namespaces_view_;

    scoped_array_t<scoped_ptr_t<per_thread_ack_info_t<protocol_t> > > per_thread_info_;

    DISABLE_COPYING(ack_info_t);
};

/* This is in part because these types aren't copyable so they can't go in
 * a std::pair. This class is used to hold a reactor and a watchable that
 * it's watching. */
template <class protocol_t>
class watchable_and_reactor_t : private master_t<protocol_t>::ack_checker_t {
public:
    watchable_and_reactor_t(io_backender_t *io_backender,
                            reactor_driver_t<protocol_t> *parent,
                            namespace_id_t namespace_id,
                            int64_t _cache_size,
                            const blueprint_t<protocol_t> &bp,
                            svs_by_namespace_t<protocol_t> *svs_by_namespace,
                            typename protocol_t::context_t *_ctx) :
        watchable(bp),
        ctx(_ctx),
        parent_(parent),
        namespace_id_(namespace_id),
        svs_by_namespace_(svs_by_namespace),
        cache_size(_cache_size)
    {
        coro_t::spawn_sometime(boost::bind(&watchable_and_reactor_t<protocol_t>::initialize_reactor, this, io_backender));
    }

    ~watchable_and_reactor_t() {
        /* Make sure that the coro we spawn to initialize this things has
         * actually run. */
        reactor_has_been_initialized_.wait_lazily_unordered();

        reactor_directory_subscription_.reset();
        {
            mutex_assertion_t::acq_t acq(&parent_->watchable_variable_lock);
            namespaces_directory_metadata_t<protocol_t> directory = parent_->watchable_variable.get_watchable()->get();
            size_t num_erased = directory.reactor_bcards.erase(namespace_id_);
            guarantee(num_erased == 1);
            parent_->watchable_variable.set_value(directory);
        }

        /* Destroy the reactor. (Dun dun duhnnnn...) */
        reactor_.reset();
    }

    static bool compute_is_acceptable_ack_set(const std::set<peer_id_t> &acks, const namespace_id_t &namespace_id, per_thread_ack_info_t<protocol_t> *ack_info) {
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
        cow_ptr_t<namespaces_semilattice_metadata_t<protocol_t> > nmd = ack_info->get_namespaces_view();

        for (std::set<peer_id_t>::const_iterator it = acks.begin(); it != acks.end(); it++) {
            std::map<peer_id_t, machine_id_t>::iterator tt_it = translation_table_snapshot.find(*it);
            if (tt_it == translation_table_snapshot.end()) continue;
            machines_semilattice_metadata_t::machine_map_t::iterator jt = mmd.machines.find(tt_it->second);
            if (jt == mmd.machines.end()) continue;
            if (jt->second.is_deleted()) continue;
            if (jt->second.get().datacenter.in_conflict()) continue;
            datacenter_id_t dc = jt->second.get().datacenter.get();
            acks_by_dc.insert(dc);
        }
        typename namespaces_semilattice_metadata_t<protocol_t>::namespace_map_t::const_iterator it =
            nmd->namespaces.find(namespace_id);
        if (it == nmd->namespaces.end()) return false;
        if (it->second.is_deleted()) return false;
        if (it->second.get().ack_expectations.in_conflict()) return false;
        std::map<datacenter_id_t, int> expected_acks = it->second.get().ack_expectations.get();

        /* The nil uuid represents acks from anywhere. */
        int extra_acks = 0;
        for (std::map<datacenter_id_t, int>::const_iterator kt = expected_acks.begin(); kt != expected_acks.end(); ++kt) {
            if (!kt->first.is_nil()) {
                if (acks_by_dc.count(kt->first) < static_cast<size_t>(kt->second)) {
                    return false;
                }
                extra_acks += acks_by_dc.count(kt->first) - static_cast<size_t>(kt->second);
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

        if (extra_acks < expected_acks[nil_uuid()]) {
            return false;
        }
        return true;
    }

    bool is_acceptable_ack_set(const std::set<peer_id_t> &acks) {
        return compute_is_acceptable_ack_set(acks, namespace_id_, parent_->ack_info->per_thread_ack_info());
    }

private:
    std::map<peer_id_t, boost::optional<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t<protocol_t> > > > > extract_reactor_directory(
            const std::map<peer_id_t, namespaces_directory_metadata_t<protocol_t> > &nss) {
        std::map<peer_id_t, boost::optional<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t<protocol_t> > > > > out;
        for (typename std::map<peer_id_t, namespaces_directory_metadata_t<protocol_t> >::const_iterator it = nss.begin(); it != nss.end(); it++) {
            typename std::map<namespace_id_t, directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t<protocol_t> > > >::const_iterator jt =
                it->second.reactor_bcards.find(namespace_id_);
            if (jt == it->second.reactor_bcards.end()) {
                out.insert(std::make_pair(it->first, boost::optional<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t<protocol_t> > > >()));
            } else {
                out.insert(std::make_pair(it->first, boost::optional<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t<protocol_t> > > >(jt->second)));
            }
        }
        return out;
    }

    void on_change_reactor_directory() {
        mutex_assertion_t::acq_t acq(&parent_->watchable_variable_lock);
        namespaces_directory_metadata_t<protocol_t> directory = parent_->watchable_variable.get_watchable()->get();
        directory.reactor_bcards.find(namespace_id_)->second = reactor_->get_reactor_directory()->get();
        parent_->watchable_variable.set_value(directory);
    }

    void initialize_reactor(io_backender_t *io_backender) {
        perfmon_collection_repo_t::collections_t *perfmon_collections = parent_->perfmon_collection_repo->get_perfmon_collections_for_namespace(namespace_id_);
        perfmon_collection_t *namespace_collection = &perfmon_collections->namespace_collection;
        perfmon_collection_t *serializers_collection = &perfmon_collections->serializers_collection;

        // TODO: We probably shouldn't have to pass in this perfmon collection.
        svs_by_namespace_->get_svs(serializers_collection, namespace_id_, cache_size, &stores_lifetimer_, &svs_, ctx);

        reactor_.init(new reactor_t<protocol_t>(
            io_backender,
            parent_->mbox_manager,
            this,
            parent_->directory_view->subview(boost::bind(&watchable_and_reactor_t<protocol_t>::extract_reactor_directory, this, _1)),
            parent_->branch_history_manager,
            watchable.get_watchable(),
            svs_.get(), namespace_collection, ctx));

        {
            typename watchable_t<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t<protocol_t> > > >::freeze_t reactor_directory_freeze(reactor_->get_reactor_directory());
            reactor_directory_subscription_.init(
                new typename watchable_t<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t<protocol_t> > > >::subscription_t(
                    boost::bind(&watchable_and_reactor_t<protocol_t>::on_change_reactor_directory, this),
                    reactor_->get_reactor_directory(), &reactor_directory_freeze));
            mutex_assertion_t::acq_t acq(&parent_->watchable_variable_lock);
            namespaces_directory_metadata_t<protocol_t> directory = parent_->watchable_variable.get_watchable()->get();
            std::pair<typename std::map<namespace_id_t, directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t<protocol_t> > > >::iterator, bool> insert_res
                = directory.reactor_bcards.insert(std::make_pair(namespace_id_, reactor_->get_reactor_directory()->get()));
            guarantee(insert_res.second);  // Ensure a value did not already exist.
            parent_->watchable_variable.set_value(directory);
        }

        reactor_has_been_initialized_.pulse();
    }

public:
    watchable_variable_t<blueprint_t<protocol_t> > watchable;

    typename protocol_t::context_t *const ctx;

private:
    cond_t reactor_has_been_initialized_;

    reactor_driver_t<protocol_t> *const parent_;
    const namespace_id_t namespace_id_;
    svs_by_namespace_t<protocol_t> *const svs_by_namespace_;

    stores_lifetimer_t<protocol_t> stores_lifetimer_;
    scoped_ptr_t<multistore_ptr_t<protocol_t> > svs_;
    scoped_ptr_t<reactor_t<protocol_t> > reactor_;

    scoped_ptr_t<typename watchable_t<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t<protocol_t> > > >::subscription_t> reactor_directory_subscription_;
    int64_t cache_size;

    DISABLE_COPYING(watchable_and_reactor_t);
};

template <class protocol_t>
reactor_driver_t<protocol_t>::reactor_driver_t(io_backender_t *_io_backender,
                                               mailbox_manager_t *_mbox_manager,
                                               const clone_ptr_t<watchable_t<std::map<peer_id_t, namespaces_directory_metadata_t<protocol_t> > > > &_directory_view,
                                               branch_history_manager_t<protocol_t> *_branch_history_manager,
                                               boost::shared_ptr<semilattice_readwrite_view_t<cow_ptr_t<namespaces_semilattice_metadata_t<protocol_t> > > > _namespaces_view,
                                               boost::shared_ptr<semilattice_read_view_t<machines_semilattice_metadata_t> > machines_view_,
                                               const clone_ptr_t<watchable_t<std::map<peer_id_t, machine_id_t> > > &_machine_id_translation_table,
                                               svs_by_namespace_t<protocol_t> *_svs_by_namespace,
                                               perfmon_collection_repo_t *_perfmon_collection_repo,
                                               typename protocol_t::context_t *_ctx)
    : io_backender(_io_backender),
      mbox_manager(_mbox_manager),
      directory_view(_directory_view),
      branch_history_manager(_branch_history_manager),
      machine_id_translation_table(_machine_id_translation_table),
      namespaces_view(_namespaces_view),
      machines_view(machines_view_),
      ctx(_ctx),
      svs_by_namespace(_svs_by_namespace),
      ack_info(new ack_info_t<protocol_t>(machine_id_translation_table, machines_view, namespaces_view)),
      watchable_variable(namespaces_directory_metadata_t<protocol_t>()),
      semilattice_subscription(boost::bind(&reactor_driver_t<protocol_t>::on_change, this), namespaces_view),
      translation_table_subscription(boost::bind(&reactor_driver_t<protocol_t>::on_change, this)),
      perfmon_collection_repo(_perfmon_collection_repo)
{
    watchable_t<std::map<peer_id_t, machine_id_t> >::freeze_t freeze(machine_id_translation_table);
    translation_table_subscription.reset(machine_id_translation_table, &freeze);
    on_change();
}

template<class protocol_t>
reactor_driver_t<protocol_t>::~reactor_driver_t() {
    /* This must be defined in the `.tcc` file because the full definition of
    `watchable_and_reactor_t` is not available in the `.hpp` file. */
}

template<class protocol_t>
void reactor_driver_t<protocol_t>::delete_reactor_data(
        auto_drainer_t::lock_t lock, 
        typename reactor_map_t::auto_type *thing_to_delete,
        namespace_id_t namespace_id)
{
    lock.assert_is_holding(&drainer);
    delete thing_to_delete;
    svs_by_namespace->destroy_svs(namespace_id);
}

template<class protocol_t>
void reactor_driver_t<protocol_t>::on_change() {
    cow_ptr_t<namespaces_semilattice_metadata_t<protocol_t> > namespaces = namespaces_view->get();

    for (typename namespaces_semilattice_metadata_t<protocol_t>::namespace_map_t::const_iterator 
                it =  namespaces->namespaces.begin(); it != namespaces->namespaces.end(); it++) {
        if (it->second.is_deleted() && std_contains(reactor_data, it->first)) {
            /* on_change cannot block because it is called as part of
             * semilattice subscription, however the
             * watchable_and_reactor_t destructor can block... therefore
             * bullshit takes place. We must release a value from the
             * ptr_map into this bullshit auto_type so that it's not in the
             * map but the destructor hasn't been called... then this needs
             * to be heap allocated so that it can be safely passed to a
             * coroutine for destruction. */
            coro_t::spawn_sometime(boost::bind(
                &reactor_driver_t<protocol_t>::delete_reactor_data,
                this,
                auto_drainer_t::lock_t(&drainer),
                new typename
                    reactor_map_t::auto_type(reactor_data.release(reactor_data.find(it->first))),
                it->first));
        } else {
            persistable_blueprint_t<protocol_t> pbp;

            try {
                pbp = it->second.get().blueprint.get();
            } catch (in_conflict_exc_t) {
                //Nothing to do for this namespaces, its blueprint is in
                //conflict.
                continue;
            }

            blueprint_t<protocol_t> bp = translate_blueprint(pbp, machine_id_translation_table->get());

            if (std_contains(bp.peers_roles, mbox_manager->get_connectivity_service()->get_me())) {
                /* Either construct a new reactor (if this is a namespace we
                 * haven't seen before). Or send the new blueprint to the
                 * existing reactor. */
                if (!std_contains(reactor_data, it->first)) {
                    int64_t cache_size;
                    if (it->second.get().cache_size.in_conflict()) {
                        cache_size = GIGABYTE;
                    } else {
                        cache_size = it->second.get().cache_size.get();
                    }

                    if (cache_size < 16 * MEGABYTE) {
                        cache_size = 16 * MEGABYTE;
                        logINF("Namespace %s(%s) has too small of a cache size. Increasing it to 16 megabytes.\n",
                                uuid_to_str(it->first).c_str(),
                                it->second.get().name.in_conflict() ? "Name in conflict" : it->second.get().name.get().c_str());
                    }

                    if (cache_size > 64 * GIGABYTE) {
                        cache_size = 16 * GIGABYTE;
                        logINF("Namespace %s(%s) has too large of a cache size. Decreasing it to 64 gigabyes.\n",
                                uuid_to_str(it->first).c_str(),
                                it->second.get().name.in_conflict() ? "Name in conflict" : it->second.get().name.get().c_str());
                    }

                    namespace_id_t tmp = it->first;
                    reactor_data.insert(tmp, new watchable_and_reactor_t<protocol_t>(io_backender, this, it->first, cache_size, bp, svs_by_namespace, ctx));
                } else {
                    reactor_data.find(it->first)->second->watchable.set_value(bp);
                }
            } else {
                /* The blueprint does not mentions us so we destroy the
                 * reactor. */
                if (std_contains(reactor_data, it->first)) {
                    coro_t::spawn_sometime(boost::bind(&reactor_driver_t<protocol_t>::delete_reactor_data, this, auto_drainer_t::lock_t(&drainer), new typename reactor_map_t::auto_type(reactor_data.release(reactor_data.find(it->first))), it->first));
                }
            }
        }
    }
}

#endif /* CLUSTERING_ADMINISTRATION_REACTOR_DRIVER_TCC_ */
