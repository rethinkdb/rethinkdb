// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_MANAGER_TABLE_META_CLIENT_HPP_
#define CLUSTERING_TABLE_MANAGER_TABLE_META_CLIENT_HPP_

#include "clustering/table_manager/table_metadata.hpp"
#include "concurrency/cross_thread_watchable.hpp"
#include "concurrency/watchable_map.hpp"

class multi_table_manager_t;
class server_config_client_t;

/* These four exception classes are all thrown by `table_meta_client_t` to describe
different error conditions. There are several reasons why this is better than having
`table_meta_client_t` just throw an `admin_op_exc_t`:

 1. Sometimes we want to catch `no_such_table_exc_t` instead of reporting it to the user.
    For example, if we call `list_names()` and then run some other operation on each
    table in the list, we want to ignore any `no_such_table_exc_t`s thrown by that
    operation.

 2. `table_meta_client_t` usually doesn't have access to the name of the table. The
    caller will typically catch the exception at a point where the table name is known
    and then produce a user-readable error message that includes the table name.

 3. `table_meta_client_t` often doesn't have enough context to produce an optimal error
    message. The caller will typically add more context when it catches the exception and
    forwards it to the user. For example, instead of saying "the table's configuration
    was not modified" it can say "the secondary index was not renamed".

Note that the exception descriptions here are mostly just for documentation; unless there
is a bug, they will never be shown to the user. */

class no_such_table_exc_t : public std::runtime_error {
public:
    no_such_table_exc_t() :
        std::runtime_error("There is no table with the given name / UUID.") { }
};

class ambiguous_table_exc_t : public std::runtime_error {
public:
    ambiguous_table_exc_t() :
        std::runtime_error("There are multiple tables with the given name.") { }
};

class failed_table_op_exc_t : public std::runtime_error {
public:
    failed_table_op_exc_t() : std::runtime_error("The attempt to read or modify the "
        "table's configuration failed because none of the servers were accessible.  If "
        "it was an attempt to modify, the modification did not take place.") { }
};

class maybe_failed_table_op_exc_t : public std::runtime_error {
public:
    maybe_failed_table_op_exc_t() : std::runtime_error("The attempt to modify the "
        "table's configuration failed because we lost contact with the servers after "
        "initiating the modification, or the Raft leader lost contact with its "
        "followers, or we timed out while waiting for the changes to propagate.  The "
        "modification may or may not have taken place.") { }
};

class config_change_exc_t : public std::runtime_error {
public:
    config_change_exc_t() : std::runtime_error("The change could not be applied to the "
        "table's configuration.") { }
};

/* `table_meta_client_t` is responsible for submitting client requests over the network
to the `multi_table_manager_t`. It doesn't have any real state of its own; it's just a
convenient way of bundling together all of the objects that are necessary for submitting
a client request. */
class table_meta_client_t :
    public home_thread_mixin_t {
public:
    table_meta_client_t(
        mailbox_manager_t *_mailbox_manager,
        multi_table_manager_t *_multi_table_manager,
        watchable_map_t<peer_id_t, multi_table_manager_bcard_t>
            *_multi_table_manager_directory,
        watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_manager_bcard_t>
            *_table_manager_directory,
        server_config_client_t *_server_config_client);

    /* All of these functions can be called from any thread. */

    /* `find()` determines the ID of the table with the given name in the given database.
    */
    void find(
        const database_id_t &database, const name_string_t &name,
        namespace_id_t *table_id_out, std::string *primary_key_out = nullptr)
        THROWS_ONLY(no_such_table_exc_t, ambiguous_table_exc_t);

    /* `exists()` returns `true` if a table with the given `table_id` exists. */
    bool exists(const namespace_id_t &table_id);

    /* `exists()` returns `true` if one or more tables exist with the given name in the
    given database. */
    bool exists(const database_id_t &database, const name_string_t &name);

    /* `get_name()` determines the name, database, and primary key of the table with the
    given ID; it's the reverse of `find()`. It will not block. */
    void get_name(
        const namespace_id_t &table_id,
        table_basic_config_t *basic_config_out)
        THROWS_ONLY(no_such_table_exc_t);

    /* `list_names()` determines the IDs, names, databases, and primary keys of every
    table. It will not block. */
    void list_names(
        std::map<namespace_id_t, table_basic_config_t> *names_out) const;

    /* `get_config()` fetches the configuration of the table with the given ID. It may
    block and it may fail. */
    void get_config(
        const namespace_id_t &table_id,
        signal_t *interruptor,
        table_config_and_shards_t *config_out)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, failed_table_op_exc_t);

    /* `list_configs()` fetches the configurations of every table at once. It may block.
    If it can't find a config for a certain table, then it puts the table's name and info
    into `disconnected_configs_out` instead. */
    void list_configs(
        signal_t *interruptor,
        std::map<namespace_id_t, table_config_and_shards_t> *configs_out,
        std::map<namespace_id_t, table_basic_config_t> *disconnected_configs_out)
        THROWS_ONLY(interrupted_exc_t);

    /* `get_sindex_status()` returns a list of the sindexes on the given table and the
    status of each one. */
    void get_sindex_status(
        const namespace_id_t &table_id,
        signal_t *interruptor,
        std::map<std::string, std::pair<sindex_config_t, sindex_status_t> >
            *index_statuses_out)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, failed_table_op_exc_t);

    /* `get_shard_status()` returns some of the information necessary to fill in the
    `rethinkdb.table_status` system table. If `server_shards_out` is set to `nullptr`, it
    that information will not be retrieved, which will improve performance. */
    void get_shard_status(
        const namespace_id_t &table_id,
        all_replicas_ready_mode_t all_replicas_ready_mode,
        signal_t *interruptor,
        std::map<server_id_t, range_map_t<key_range_t::right_bound_t,
            table_shard_status_t> > *shard_statuses_out,
        bool *all_replicas_ready_out)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, failed_table_op_exc_t);

    /* `get_raft_leader()` fetches raft leader from the table directory.
    This is for displaying raft information in `rethinkdb.table_status`. */
    void get_raft_leader(
        const namespace_id_t &table_id,
        signal_t *interruptor,
        boost::optional<server_id_t> *raft_leader_out)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, failed_table_op_exc_t);

    /* `get_debug_status()` fetches all status information from all servers. This is for
    displaying in `rethinkdb._debug_table_status`. */
    void get_debug_status(
        const namespace_id_t &table_id,
        all_replicas_ready_mode_t all_replicas_ready_mode,
        signal_t *interruptor,
        std::map<server_id_t, table_status_response_t> *responses_out)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, failed_table_op_exc_t);

    /* `create()` creates a table with the given configuration. It sets `*table_id_out`
    to the ID of the newly generated table. It may block. If it returns successfully, the
    change will be visible in `find()`, etc. */
    void create(
        namespace_id_t new_table_id,
        const table_config_and_shards_t &new_config,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, failed_table_op_exc_t,
            maybe_failed_table_op_exc_t);

    /* `drop()` drops the table with the given ID. It may block. As long as the table
    exists it will always succeed, even if the other servers are not accessible. */
    void drop(
        const namespace_id_t &table_id,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t);

    /* `set_config()` changes the configuration of the table with the given ID. It may
    block. If it returns successfully, the change will be visible in `find()`, etc. */
    void set_config(
        const namespace_id_t &table_id,
        const table_config_and_shards_change_t &table_config_and_shards_change,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, failed_table_op_exc_t,
            maybe_failed_table_op_exc_t, config_change_exc_t);

    /* `emergency_repair()` performs an emergency repair operation on the given table,
    creating a new table epoch. If all of the replicas for a given shard are missing, it
    will leave the shard alone if `allow_erase` is `false`, or replace the shard with
    a new empty shard if `allow_erase` is `true`. If `dry_run` is `true` it will
    compute the repair operation but not actually apply it. `simple_errors_found_out`
    and `data_loss_found_out` will indicate whether the two types of errors were
    detected. */
    void emergency_repair(
        const namespace_id_t &table_id,
        emergency_repair_mode_t mode,
        bool dry_run,
        signal_t *interruptor,
        table_config_and_shards_t *new_config_out,
        bool *erase_found_out,
        bool *rollback_found_out)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, failed_table_op_exc_t,
            maybe_failed_table_op_exc_t);

private:
    typedef std::pair<table_basic_config_t, multi_table_manager_timestamp_t>
        timestamped_basic_config_t;

    /* `create_or_emergency_repair()` factors out the common parts of `create()` and
    `emergency_repair()`. */
    void create_or_emergency_repair(
        const namespace_id_t &table_id,
        const table_raft_state_t &raft_state,
        const multi_table_manager_timestamp_t::epoch_t &epoch,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, failed_table_op_exc_t,
            maybe_failed_table_op_exc_t);

    /* `get_status()` runs a status query. It can run for a specific table or every
    table. If `servers` is `EVERY_SERVER`, it runs against every server for the table(s);
    if `BEST_SERVER_ONLY`, it only runs against one server for each table, which will be
    the most up-to-date server that can be found. If it fails to contact at least one
    server for a given table, it calls `failure_callback()` on that table. If you specify
    a particular table and that table doesn't exist, it throws `no_such_table_exc_t`. */
    enum class server_selector_t { EVERY_SERVER, BEST_SERVER_ONLY };
    void get_status(
        const boost::optional<namespace_id_t> &table,
        const table_status_request_t &request,
        server_selector_t servers,
        signal_t *interruptor,
        const std::function<void(
            const server_id_t &server,
            const namespace_id_t &table,
            const table_status_response_t &response
            )> &callback,
        std::set<namespace_id_t> *failures_out)
        THROWS_ONLY(interrupted_exc_t);

    /* `retry()` calls `fun()` repeatedly. If `fun()` fails with a
    `failed_table_op_exc_t` or `maybe_failed_table_op_exc_t`, then `retry()` catches the
    exception, waits for some time, and calls `fun()` again. After some number of tries
    it gives up and allows the exception to bubble up to the caller. */
    void retry(const std::function<void(signal_t *)> &fun, signal_t *interruptor);

    NORETURN void throw_appropriate_exception(const namespace_id_t &table_id)
        THROWS_ONLY(no_such_table_exc_t, failed_table_op_exc_t);

    void wait_until_change_visible(
        const namespace_id_t &table_id,
        const std::function<bool(const timestamped_basic_config_t *)> &cb,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, maybe_failed_table_op_exc_t);

    mailbox_manager_t *const mailbox_manager;
    multi_table_manager_t *const multi_table_manager;
    watchable_map_t<peer_id_t, multi_table_manager_bcard_t>
        *const multi_table_manager_directory;
    watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_manager_bcard_t>
        *const table_manager_directory;
    server_config_client_t *const server_config_client;

    /* `table_basic_configs` distributes the `table_basic_config_t`s from the
    `multi_table_manager_t` to each thread, so that `find()`, `get_name()`, and
    `list_names()` can run without blocking. */
    all_thread_watchable_map_var_t<namespace_id_t, timestamped_basic_config_t>
        table_basic_configs;
};

#endif /* CLUSTERING_TABLE_MANAGER_TABLE_META_CLIENT_HPP_ */

