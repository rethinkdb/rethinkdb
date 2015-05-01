// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_MANAGER_TABLE_META_CLIENT_HPP_
#define CLUSTERING_TABLE_MANAGER_TABLE_META_CLIENT_HPP_

#include "clustering/table_manager/table_metadata.hpp"
#include "concurrency/cross_thread_watchable.hpp"
#include "concurrency/watchable_map.hpp"

class multi_table_manager_t;

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
        "table's configuration failed because none of the servers were accessible") { }
};

class maybe_failed_table_op_exc_t : public std::runtime_error {
public:
    maybe_failed_table_op_exc_t() : std::runtime_error("something went wrong while we "
        "were trying to modify the table's configuration; the modification may or may "
        "not have been applied") { }
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

    /* `exists()` returns `true` if one or more tables exist with the given name in the
    given database. */
    bool exists(const database_id_t &database, const name_string_t &name);

    /* `get_name()` determines the name and database of the table with the given ID; it's
    the reverse of `find()`. It returns `false` if there is no existing table with that
    ID. `get_name()` will not block. */
    void get_name(
        const namespace_id_t &table_id,
        database_id_t *db_out,
        name_string_t *name_out)
        THROWS_ONLY(no_such_table_exc_t);

    /* `list_names()` determines the IDs, names, and databases of every table. It will
    not block. */
    void list_names(
        std::map<namespace_id_t, std::pair<database_id_t, name_string_t> > *names_out);

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

    /* `get_status()` returns the secondary index status of the table with the given ID.
    It may block. */
    void get_status(
        const namespace_id_t &table_id,
        signal_t *interruptor,
        std::map<std::string, std::pair<sindex_config_t, sindex_status_t> > *res_out)
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

private:
    typedef std::pair<table_basic_config_t, multi_table_manager_bcard_t::timestamp_t>
        timestamped_basic_config_t;

    [[noreturn]] void throw_appropriate_exception(const namespace_id_t &table_id)
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

