// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/reactor_driver.hpp"

#include <map>
#include <set>
#include <utility>

#include "errors.hpp"
#include <boost/bind.hpp>

#include "clustering/administration/metadata.hpp"
#include "clustering/administration/perfmon_collection_repo.hpp"
#include "clustering/administration/servers/server_id_to_peer_id.hpp"
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

table_directory_converter_t::table_directory_converter_t(
        watchable_map_t<std::pair<peer_id_t, namespace_id_t>,
                        namespace_directory_metadata_t> *_directory,
        namespace_id_t _table_id) :
    watchable_map_transform_t(_directory),
    table_id(_table_id) { }

bool table_directory_converter_t::key_1_to_2(
        const std::pair<peer_id_t, namespace_id_t> &key1,
        peer_id_t *key2_out) {
    if (key1.second == table_id) {
        *key2_out = key1.first;
        return true;
    } else {
        return false;
    }
}

void table_directory_converter_t::value_1_to_2(
        const namespace_directory_metadata_t *value1,
        const namespace_directory_metadata_t **value2_out) {
    *value2_out = value1;
}

bool table_directory_converter_t::key_2_to_1(
        const peer_id_t &key2,
        std::pair<peer_id_t, namespace_id_t> *key1_out) {
    key1_out->first = key2;
    key1_out->second = table_id;
    return true;
}

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

bool stores_lifetimer_t::is_gc_active() const {
    if (serializer_.has()) {
        return serializer_->is_gc_active();
    } else {
        return false;
    }
}

stores_lifetimer_t::sindex_jobs_t stores_lifetimer_t::get_sindex_jobs() const {
    stores_lifetimer_t::sindex_jobs_t sindex_jobs;

    if (stores_.has()) {
        for (size_t i = 0; i < stores_.size(); ++i) {
            if (stores_[i].has()) {
                for (auto const &job : *(stores_[i]->get_sindex_jobs())) {
                    sindex_jobs.insert(std::make_pair(
                        //             `uuid_u`,                   `std::string`
                        std::make_pair(stores_[i]->get_table_id(), job.second.second),
                        job.second.first));
                }
            }
        }
    }

    return sindex_jobs;
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
servers. This is a hack, but it ensures that we produce a blueprint that tricks the
`reactor_t` into doing what we want it to. `blueprint_name_translator_t` takes care of
automatically generating and keeping track of the fakes.

TODO: This is pretty hacky. Eventually the `reactor_t` will deal with server IDs
directly. */
class blueprint_id_translator_t {
public:
    explicit blueprint_id_translator_t(server_config_client_t *server_config_client) :
        server_id_to_peer_id_map(
            server_config_client->get_server_id_to_peer_id_map()->get())
        { }
    peer_id_t server_id_to_peer_id(const server_id_t &server_id) {
        auto it = server_id_to_peer_id_map.find(server_id);
        if (it == server_id_to_peer_id_map.end()) {
            it = server_id_to_peer_id_map.insert(
                std::make_pair(server_id, peer_id_t(generate_uuid()))).first;
        }
        return it->second;
    }
private:
    std::map<server_id_t, peer_id_t> server_id_to_peer_id_map;
};

blueprint_t construct_blueprint(const table_replication_info_t &info,
                                server_config_client_t *server_config_client) {
    rassert(info.config.shards.size() ==
        static_cast<size_t>(info.shard_scheme.num_shards()));

    blueprint_id_translator_t trans(server_config_client);

    blueprint_t blueprint;

    /* Put the primaries in the blueprint */
    for (size_t i = 0; i < info.config.shards.size(); ++i) {
        peer_id_t peer;
        if (!static_cast<bool>(server_config_client->get_name_for_server_id(
                info.config.shards[i].primary_replica))) {
            /* The server was permanently removed. `table_config` will show `null` in
            the `primary_replica` field. Pick a random peer ID to make sure that the
            table acts as though the primary replica is unavailable. */
            peer = peer_id_t(generate_uuid());
        } else {
            peer = trans.server_id_to_peer_id(info.config.shards[i].primary_replica);
        }
        if (blueprint.peers_roles.count(peer) == 0) {
            blueprint.add_peer(peer);
        }
        blueprint.add_role(
            peer,
            hash_region_t<key_range_t>(info.shard_scheme.get_shard_range(i)),
            blueprint_role_primary);
    }

    /* Put the secondary replicas in the blueprint */
    for (size_t i = 0; i < info.config.shards.size(); ++i) {
        const table_config_t::shard_t &shard = info.config.shards[i];
        for (const server_id_t &server : shard.replicas) {
            if (!static_cast<bool>(
                    server_config_client->get_name_for_server_id(server))) {
                /* The server was permanently removed. It won't appear in the list of
                replicas shown in `table_config` or `table_status`. Act as though we
                never saw it. */
                continue;
            }
            peer_id_t peer = trans.server_id_to_peer_id(server);
            if (blueprint.peers_roles.count(peer) == 0) {
                blueprint.add_peer(peer);
            }
            if (server != shard.primary_replica) {
                blueprint.add_role(
                    peer,
                    hash_region_t<key_range_t>(info.shard_scheme.get_shard_range(i)),
                    blueprint_role_secondary);
            }
        }
    }

    /* Make sure that every known peer appears in the blueprint in some form, so that the
    reactor doesn't proceed without approval of every known peer */
    std::map<server_id_t, peer_id_t> server_id_to_peer_id_map =
        server_config_client->get_server_id_to_peer_id_map()->get();
    for (auto it = server_id_to_peer_id_map.begin();
              it != server_id_to_peer_id_map.end();
            ++it) {
        peer_id_t peer = trans.server_id_to_peer_id(it->first);
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
class watchable_and_reactor_t :
        private ack_checker_t {
public:
    watchable_and_reactor_t(const base_path_t &_base_path,
                            io_backender_t *io_backender,
                            reactor_driver_t *parent,
                            namespace_id_t namespace_id,
                            const blueprint_t &blueprint,
                            const table_replication_info_t &repli_info,
                            const servers_semilattice_metadata_t &server_md,
                            svs_by_namespace_t *svs_by_namespace,
                            rdb_context_t *_ctx) :
        table_directory(parent->directory_view, namespace_id),
        base_path(_base_path),
        watchable(blueprint),
        ctx(_ctx),
        parent_(parent),
        namespace_id_(namespace_id),
        svs_by_namespace_(svs_by_namespace),
        write_ack_config_var(write_ack_config_checker_t(repli_info.config, server_md)),
        write_durability_var(repli_info.config.durability),
        write_ack_config_cross_threader(write_ack_config_var.get_watchable()),
        write_durability_cross_threader(write_durability_var.get_watchable())
    {
        coro_t::spawn_sometime(boost::bind(&watchable_and_reactor_t::initialize_reactor, this, io_backender));
    }

    ~watchable_and_reactor_t() {
        /* Make sure that the coro we spawn to initialize this things has
         * actually run. */
        reactor_has_been_initialized_.wait_lazily_unordered();

        /* XXX the order in which the perform the operations is important and
         * will cause bugs if any changes are made. */

        /* We have to destroy `directory_exporter_` before `reactor_` because
        `directory_exporter_` is subscribed to a `watchable_t` that `reactor_` owns. */
        directory_exporter_.reset();

        /* Destroy the reactor. (Dun dun duhnnnn...). Next we destroy the
         * reactor. We need to do this before we remove the reactor bcard. This
         * is because there exists parts of the be_[role] (be_primary,
         * be_secondary etc.) function which assume that the reactors own bcard
         * will be in place for their duration.
        TODO: We should make the reactor not make that assumption. This might be easier
        after we implement Raft. */
        reactor_.reset();

        /* Finally we remove the reactor bcard. */
        parent_->watchable_var.delete_key(namespace_id_);
    }

    void update_repli_info(const blueprint_t &blueprint,
                           const table_replication_info_t &repli_info,
                           const servers_semilattice_metadata_t &server_md) {
        watchable.set_value(blueprint);
        write_ack_config_var.set_value_no_equals(
            write_ack_config_checker_t(repli_info.config, server_md));
        write_durability_var.set_value(repli_info.config.durability);
    }

    bool is_acceptable_ack_set(const std::set<server_id_t> &acks) const {
        bool ok;
        write_ack_config_cross_threader.get_watchable()->apply_read(
            [&](const write_ack_config_checker_t *checker) {
                ok = checker->check_acks(acks);
            });
        return ok;
    }

    write_durability_t get_write_durability() const {
        return write_durability_cross_threader.get_watchable()->get();
    }

    bool is_gc_active() const {
        return stores_lifetimer_.is_gc_active();
    }

    stores_lifetimer_t::sindex_jobs_t get_sindex_jobs() const {
        return stores_lifetimer_.get_sindex_jobs();
    }

    typedef std::map<std::pair<namespace_id_t, region_t>, reactor_progress_report_t>
        backfill_progress_t;
    backfill_progress_t get_backfill_progress() const {
        backfill_progress_t backfill_progress;

        if (reactor_.has()) {
            for (auto const &backfill : reactor_->get_progress()) {
                backfill_progress.insert(
                    std::make_pair(std::make_pair(namespace_id_, backfill.first),
                                   backfill.second));
            }
        }

        return backfill_progress;
    }

private:
    void initialize_reactor(io_backender_t *io_backender) {
        perfmon_collection_repo_t::collections_t *perfmon_collections = parent_->perfmon_collection_repo->get_perfmon_collections_for_namespace(namespace_id_);
        perfmon_collection_t *namespace_collection = &perfmon_collections->namespace_collection;
        perfmon_collection_t *serializers_collection = &perfmon_collections->serializers_collection;

        // TODO: We probably shouldn't have to pass in this perfmon collection.
        svs_by_namespace_->get_svs(serializers_collection, namespace_id_, &stores_lifetimer_, &svs_, ctx);

        reactor_.init(new reactor_t(
            base_path,
            io_backender,
            parent_->mbox_manager,
            parent_->server_id,
            &parent_->backfill_throttler,
            this,
            &table_directory,
            parent_->branch_history_manager,
            watchable.get_watchable(),
            svs_.get(), namespace_collection, ctx));

        directory_exporter_.init(
            new watchable_map_entry_copier_t<
                    namespace_id_t, namespace_directory_metadata_t>(
                &parent_->watchable_var,
                namespace_id_,
                reactor_->get_reactor_directory(),
                /* `directory_exporter_` shouldn't immediately remove the directory entry
                when it's deleted; see note in `~watchable_and_reactor_t()`. */
                false
            ));

        reactor_has_been_initialized_.pulse();
    }

    table_directory_converter_t table_directory;
    const base_path_t base_path;
    cond_t reactor_has_been_initialized_;
    watchable_variable_t<blueprint_t> watchable;
    rdb_context_t *const ctx;

    reactor_driver_t *const parent_;
    const namespace_id_t namespace_id_;
    svs_by_namespace_t *const svs_by_namespace_;

    watchable_variable_t<write_ack_config_checker_t> write_ack_config_var;
    watchable_variable_t<write_durability_t> write_durability_var;
    all_thread_watchable_variable_t<write_ack_config_checker_t>
        write_ack_config_cross_threader;
    all_thread_watchable_variable_t<write_durability_t>
        write_durability_cross_threader;

    stores_lifetimer_t stores_lifetimer_;
    scoped_ptr_t<multistore_ptr_t> svs_;
    scoped_ptr_t<reactor_t> reactor_;

    scoped_ptr_t<watchable_map_entry_copier_t<
        namespace_id_t, namespace_directory_metadata_t> > directory_exporter_;

    DISABLE_COPYING(watchable_and_reactor_t);
};

reactor_driver_t::reactor_driver_t(
    const base_path_t &_base_path,
    io_backender_t *_io_backender,
    mailbox_manager_t *_mbox_manager,
    const server_id_t &_server_id,
    watchable_map_t<
        std::pair<peer_id_t, namespace_id_t>,
        namespace_directory_metadata_t
        > *_directory_view,
    branch_history_manager_t *_branch_history_manager,
    boost::shared_ptr< semilattice_readwrite_view_t<
        cluster_semilattice_metadata_t> > _semilattice_view,
    server_config_client_t *_server_config_client,
    signal_t *_we_were_permanently_removed,
    svs_by_namespace_t *_svs_by_namespace,
    perfmon_collection_repo_t *_perfmon_collection_repo,
    rdb_context_t *_ctx)
    : base_path(_base_path),
      io_backender(_io_backender),
      mbox_manager(_mbox_manager),
      server_id(_server_id),
      directory_view(_directory_view),
      branch_history_manager(_branch_history_manager),
      semilattice_view(_semilattice_view),
      server_config_client(_server_config_client),
      we_were_permanently_removed(_we_were_permanently_removed),
      ctx(_ctx),
      svs_by_namespace(_svs_by_namespace),
      semilattice_subscription(
        boost::bind(&reactor_driver_t::on_change, this), semilattice_view),
      name_to_server_id_subscription(
        boost::bind(&reactor_driver_t::on_change, this)),
      server_id_to_peer_id_subscription(
        boost::bind(&reactor_driver_t::on_change, this)),
      perfmon_collection_repo(_perfmon_collection_repo)
{
    watchable_t< std::multimap<name_string_t, server_id_t> >::freeze_t
        name_to_server_id_freeze(
            server_config_client->get_name_to_server_id_map());
    name_to_server_id_subscription.reset(
        server_config_client->get_name_to_server_id_map(),
        &name_to_server_id_freeze);
    watchable_t< std::map<server_id_t, peer_id_t> >::freeze_t
        server_id_to_peer_id_freeze(
            server_config_client->get_server_id_to_peer_id_map());
    server_id_to_peer_id_subscription.reset(
        server_config_client->get_server_id_to_peer_id_map(),
        &server_id_to_peer_id_freeze);
    on_change();
}

reactor_driver_t::~reactor_driver_t() {
    /* This must be defined in the `.cc` file because the full definition of
    `watchable_and_reactor_t` is not available in the `.hpp` file. */
}

bool reactor_driver_t::is_gc_active() const {
    for (auto const &reactor : reactor_data) {
        if (reactor.second->is_gc_active()) {
            return true;
        }
    }

    return false;
}

reactor_driver_t::sindex_jobs_t reactor_driver_t::get_sindex_jobs() const {
    reactor_driver_t::sindex_jobs_t sindex_jobs;

    for (auto const &reactor : reactor_data) {
        if (reactor.second.has()) {
            auto reactor_sindex_jobs = reactor.second->get_sindex_jobs();
            sindex_jobs.insert(std::make_move_iterator(reactor_sindex_jobs.begin()),
                               std::make_move_iterator(reactor_sindex_jobs.end()));
        }
    }

    return sindex_jobs;
}

reactor_driver_t::backfill_progress_t reactor_driver_t::get_backfill_progress() const {
    reactor_driver_t::backfill_progress_t backfill_progress;

    for (auto const &reactor : reactor_data) {
        if (reactor.second.has()) {
            auto reactor_backfill_progress = reactor.second->get_backfill_progress();
            backfill_progress.insert(
                std::make_move_iterator(reactor_backfill_progress.begin()),
                std::make_move_iterator(reactor_backfill_progress.end()));
        }
    }

    return backfill_progress;
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
    cluster_semilattice_metadata_t md = semilattice_view->get();

    for (auto it = md.rdb_namespaces->namespaces.begin();
            it != md.rdb_namespaces->namespaces.end();
            ++it) {
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
                bp = construct_blueprint(*repli_info, server_config_client);
            } catch (server_name_collision_exc_t) {
                /* Leave the blueprint the way it was before. The user should fix their
                name collision. This is a bit hacky and it might confuse the user, but
                it's a safe option, and name collisions are rare. */
                continue;
            }
            if (!std_contains(bp.peers_roles,
                              mbox_manager->get_connectivity_cluster()->get_me())) {
                /* This can occur because there is a brief period during startup where
                our server ID might not appear in `server_config_client`'s mapping of
                server IDs and peer IDs. We just ignore it; in a moment, the mapping
                will be updated to include us and `on_change()` will run again. */
                continue;
            }

            /* Either construct a new reactor (if this is a namespace we
             * haven't seen before). Or send the new blueprint to the
             * existing reactor. */
            if (!std_contains(reactor_data, it->first)) {
                namespace_id_t tmp = it->first;
                reactor_data.insert(std::make_pair(tmp,
                    make_scoped<watchable_and_reactor_t>(
                        base_path, io_backender, this, it->first, bp,
                        *repli_info, md.servers, svs_by_namespace, ctx)));
            } else {
                reactor_data.find(it->first)->second->update_repli_info(
                    bp, *repli_info, md.servers);
            }
        }
    }
}
