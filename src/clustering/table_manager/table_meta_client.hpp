// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_MANAGER_TABLE_META_CLIENT_HPP_
#define CLUSTERING_TABLE_MANAGER_TABLE_META_CLIENT_HPP_

#include "clustering/table_manager/table_metadata.hpp"
#include "concurrency/cross_thread_watchable.hpp"
#include "concurrency/watchable_map.hpp"

class multi_table_manager_t;

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
        std::runtime_error("there is no table with the given name / UUID") { }
};

class ambiguous_table_exc_t : public std::runtime_error {
public:
    ambiguous_table_exc_t() :
        std::runtime_error("there are multiple tables with the given name") { }
};

class failed_table_op_exc_t : public std::runtime_error {
public:
    failed_table_op_exc_t() : std::runtime_error("the attempt to read or modify the "
        "table's configuration failed because none of the servers were accessible. if "
        "it was an attempt to modify, the modification did not take place.") { }
};

class maybe_failed_table_op_exc_t : public std::runtime_error {
public:
    maybe_failed_table_op_exc_t() : std::runtime_error("the attempt to modify the "
        "table's configuration failed because we lost contact with the servers after "
        "initiating the modification, or the Raft leader lost contact with its "
        "followers, or we timed out while waiting for the changes to propagate. the "
        "modification may or may not have taken place.") { }
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
            *_table_manager_directory);

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
    given ID; it's the reverse of `find()`. It returns `false` if there is no existing
    table with that ID. `get_name()` will not block. */
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
    */
    void list_configs(
        signal_t *interruptor,
        std::map<namespace_id_t, table_config_and_shards_t> *configs_out)
        THROWS_ONLY(interrupted_exc_t, failed_table_op_exc_t);

    /* `get_status()` returns detailed information about the table with the given ID:
      - A list of the secondary indexes on the table and the status of each one.
      - For each server, the server's Raft state and contract acks.
      - Which server has the most up-to-date Raft state.
    It may block. */
    void get_status(
        const namespace_id_t &table_id,
        signal_t *interruptor,
        std::map<std::string, std::pair<sindex_config_t, sindex_status_t> >
            *index_statuses_out,
        std::map<server_id_t, contracts_and_contract_acks_t> *contracts_and_acks_out,
        server_id_t *latest_server_out)
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

    /* `drop()` drops the table with the given ID. It may block. If it returns `false`,
    the change may or may not have succeeded. If it returns successfully, the change will
    be visible in `find()`, etc. */
    void drop(
        const namespace_id_t &table_id,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, failed_table_op_exc_t,
            maybe_failed_table_op_exc_t);

    /* `set_config()` changes the configuration of the table with the given ID. It may
    block. If it returns successfully, the change will be visible in `find()`, etc. */
    void set_config(
        const namespace_id_t &table_id,
        const table_config_and_shards_t &new_config,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, failed_table_op_exc_t,
            maybe_failed_table_op_exc_t);

    /* `emergency_repair()` performs an emergency repair operation on the given table,
    creating a new table epoch. If all of the replicas for a given shard are missing, it
    will leave the shard alone if `allow_data_loss` is `true`, or replace the shard with
    a new empty shard if `allow_data_loss` is `false`. If `dry_run` is `true` it will
    compute the repair operation but not actually apply it. `simple_errors_found_out`
    and `data_loss_found_out` will indicate whether the two types of errors were
    detected. */
    void emergency_repair(
        const namespace_id_t &table_id,
        bool allow_data_loss,
        bool dry_run,
        signal_t *interruptor,
        table_config_and_shards_t *new_config_out,
        bool *quorum_loss_found_out,
        bool *data_loss_found_out)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t, failed_table_op_exc_t,
            maybe_failed_table_op_exc_t);

private:
    typedef std::pair<table_basic_config_t, multi_table_manager_bcard_t::timestamp_t>
        timestamped_basic_config_t;

    /* `create_or_emergency_repair()` factors out the common parts of `create()` and
    `emergency_repair()`. */
    void create_or_emergency_repair(
        const namespace_id_t &table_id,
        const table_raft_state_t &raft_state,
        microtime_t epoch_timestamp,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, failed_table_op_exc_t,
            maybe_failed_table_op_exc_t);

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

    /* `table_basic_configs` distributes the `table_basic_config_t`s from the
    `multi_table_manager_t` to each thread, so that `find()`, `get_name()`, and
    `list_names()` can run without blocking. */
    all_thread_watchable_map_var_t<namespace_id_t, timestamped_basic_config_t>
        table_basic_configs;
};

#endif /* CLUSTERING_TABLE_MANAGER_TABLE_META_CLIENT_HPP_ */

