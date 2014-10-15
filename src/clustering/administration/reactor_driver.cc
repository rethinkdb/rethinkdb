// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/reactor_driver.hpp"

#include <map>
#include <set>
#include <utility>

#include "errors.hpp"
#include <boost/bind.hpp>

#include "clustering/administration/metadata.hpp"
#include "clustering/administration/perfmon_collection_repo.hpp"
#include "clustering/administration/servers/machine_id_to_peer_id.hpp"
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

/* If the config refers to a server name for which there are multiple servers, we don't
update the blueprint until the conflict is resolved. */
class server_name_collision_exc_t : public std::exception {
public:
    const char *what() const throw () {
        return "server name collision";
    }
};

/* When constructing the blueprint, we want to generate fake peer IDs for unconnected
machines. This is a hack, but it ensures that we produce a blueprint that tricks the
`reactor_t` into doing what we want it to. `blueprint_name_translator_t` takes care of
automatically generating and keeping track of the fakes.

TODO: This is pretty hacky. Eventually the `reactor_t` will deal with machine IDs
directly. */
class blueprint_id_translator_t {
public:
    explicit blueprint_id_translator_t(server_name_client_t *name_client) :
        machine_id_to_peer_id_map(
            name_client->get_machine_id_to_peer_id_map()->get())
        { }
    peer_id_t machine_id_to_peer_id(const machine_id_t &machine_id) {
        auto it = machine_id_to_peer_id_map.find(machine_id);
        if (it == machine_id_to_peer_id_map.end()) {
            it = machine_id_to_peer_id_map.insert(
                std::make_pair(machine_id, peer_id_t(generate_uuid()))).first;
        }
        return it->second;
    }
private:
    std::map<machine_id_t, peer_id_t> machine_id_to_peer_id_map;
};

blueprint_t construct_blueprint(const table_replication_info_t &info,
                                server_name_client_t *name_client) {
    rassert(info.config.shards.size() ==
        static_cast<size_t>(info.shard_scheme.num_shards()));

    blueprint_id_translator_t trans(name_client);

    blueprint_t blueprint;

    /* Put the primaries in the blueprint */
    for (size_t i = 0; i < info.config.shards.size(); ++i) {
        peer_id_t peer = trans.machine_id_to_peer_id(info.config.shards[i].director);
        if (blueprint.peers_roles.count(peer) == 0) {
            blueprint.add_peer(peer);
        }
        blueprint.add_role(
            peer,
            hash_region_t<key_range_t>(info.shard_scheme.get_shard_range(i)),
            blueprint_role_primary);
    }

    /* Put the secondaries in the blueprint */
    for (size_t i = 0; i < info.config.shards.size(); ++i) {
        const table_config_t::shard_t &shard = info.config.shards[i];
        for (const machine_id_t &server : shard.replicas) {
            if (!static_cast<bool>(name_client->get_name_for_machine_id(server))) {
                /* The server was permanently removed. It won't appear in the list of
                replicas shown in `table_config` or `table_status`. Act as though we
                never saw it. */
                continue;
            }
            peer_id_t peer = trans.machine_id_to_peer_id(server);
            if (blueprint.peers_roles.count(peer) == 0) {
                blueprint.add_peer(peer);
            }
            if (server != shard.director) {
                blueprint.add_role(
                    peer,
                    hash_region_t<key_range_t>(info.shard_scheme.get_shard_range(i)),
                    blueprint_role_secondary);
            }
        }
    }

    /* Make sure that every known peer appears in the blueprint in some form, so that the
    reactor doesn't proceed without approval of every known peer */
    std::map<machine_id_t, peer_id_t> machine_id_to_peer_id_map =
        name_client->get_machine_id_to_peer_id_map()->get();
    for (auto it = machine_id_to_peer_id_map.begin();
              it != machine_id_to_peer_id_map.end();
            ++it) {
        peer_id_t peer = trans.machine_id_to_peer_id(it->first);
        if (blueprint.peers_roles.count(peer) == 0) {
            blueprint.add_peer(peer);
        }
    }

    /* If a peer's role isn't primary or secondary, make it nothing */
    for (size_t i = 0; i < info.config.shards.size(); ++i) {
        for (auto it = blueprint.peers_roles.begin();
                  it != blueprint.peers_roles.end();
                ++it) {
            region_t region = hash_region_t<key_range_t>(
                info.shard_scheme.get_shard_range(i));
            if (it->second.count(region) == 0) {
                blueprint.add_role(it->first, region, blueprint_role_nothing);
            }
        }
    }

    blueprint.guarantee_valid();
    return blueprint;
}

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

    bool is_acceptable_ack_set(const std::set<peer_id_t> &acks) const {
        /* RSI(reql_admin): This is temporary. When we figure out how to handle ack
        expectations in the new ReQL admin API, we'll change it. */
        return !acks.empty();
    }

    write_durability_t get_write_durability(UNUSED const peer_id_t &peer) const {
        /* RSI(reql_admin): This is temporary. When we figure out how to handle write
        durability in the new ReQL admin API, we'll change it. */
        return write_durability_t::HARD;
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

reactor_driver_t::reactor_driver_t(
    const base_path_t &_base_path,
    io_backender_t *_io_backender,
    mailbox_manager_t *_mbox_manager,
    const clone_ptr_t< watchable_t< change_tracking_map_t<peer_id_t,
        namespaces_directory_metadata_t> > > &_directory_view,
    branch_history_manager_t *_branch_history_manager,
    boost::shared_ptr< semilattice_readwrite_view_t< cow_ptr_t<
        namespaces_semilattice_metadata_t> > > _namespaces_view,
    server_name_client_t *_server_name_client,
    signal_t *_we_were_permanently_removed,
    svs_by_namespace_t *_svs_by_namespace,
    perfmon_collection_repo_t *_perfmon_collection_repo,
    rdb_context_t *_ctx)
    : base_path(_base_path),
      io_backender(_io_backender),
      mbox_manager(_mbox_manager),
      directory_view(_directory_view),
      branch_history_manager(_branch_history_manager),
      namespaces_view(_namespaces_view),
      server_name_client(_server_name_client),
      we_were_permanently_removed(_we_were_permanently_removed),
      ctx(_ctx),
      svs_by_namespace(_svs_by_namespace),
      watchable_variable(namespaces_directory_metadata_t()),
      semilattice_subscription(
        boost::bind(&reactor_driver_t::on_change, this), namespaces_view),
      name_to_machine_id_subscription(
        boost::bind(&reactor_driver_t::on_change, this)),
      machine_id_to_peer_id_subscription(
        boost::bind(&reactor_driver_t::on_change, this)),
      perfmon_collection_repo(_perfmon_collection_repo)
{
    watchable_t< std::multimap<name_string_t, machine_id_t> >::freeze_t
        name_to_machine_id_freeze(
            server_name_client->get_name_to_machine_id_map());
    name_to_machine_id_subscription.reset(
        server_name_client->get_name_to_machine_id_map(),
        &name_to_machine_id_freeze);
    watchable_t< std::map<machine_id_t, peer_id_t> >::freeze_t
        machine_id_to_peer_id_freeze(
            server_name_client->get_machine_id_to_peer_id_map());
    machine_id_to_peer_id_subscription.reset(
        server_name_client->get_machine_id_to_peer_id_map(),
        &machine_id_to_peer_id_freeze);
    on_change();
}

reactor_driver_t::~reactor_driver_t() {
    /* This must be defined in the `.cc` file because the full definition of
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

    for (namespaces_semilattice_metadata_t::namespace_map_t::const_iterator
             it = namespaces->namespaces.begin(); it != namespaces->namespaces.end(); it++) {
        if ((it->second.is_deleted() || we_were_permanently_removed->is_pulsed()) &&
                std_contains(reactor_data, it->first)) {
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
            const table_replication_info_t *repli_info =
                &it->second.get_ref().replication_info.get_ref();

            blueprint_t bp;
            try {
                bp = construct_blueprint(*repli_info, server_name_client);
            } catch (server_name_collision_exc_t) {
                /* Leave the blueprint the way it was before. The user should fix their
                name collision. This is a bit hacky and it might confuse the user, but
                it's a safe option, and name collisions are rare. */
                continue;
            }
            if (!std_contains(bp.peers_roles,
                              mbox_manager->get_connectivity_cluster()->get_me())) {
                /* This can occur because there is a brief period during startup where
                our machine ID might not appear in `server_name_client`'s mapping of
                machine IDs and peer IDs. We just ignore it; in a moment, the mapping
                will be updated to include us and `on_change()` will run again. */
                continue;
            }

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
        }
    }
}
