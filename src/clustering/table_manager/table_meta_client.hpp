// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_MANAGER_TABLE_META_CLIENT_HPP_
#define CLUSTERING_TABLE_MANAGER_TABLE_META_CLIENT_HPP_

#include "clustering/table_manager/table_metadata.hpp"
#include "concurrency/watchable_map.hpp"

/* `table_meta_client_t` is responsible for submitting client requests over the network
to the `table_meta_manager_t`. It doesn't have any real state of its own; it's just a
convenient way of bundling together all of the objects that are necessary for submitting
a client request. */
class table_meta_client_t :
    public home_thread_mixin_t {
public:
    enum class result_t {
        success,
        maybe,
        failure
    };

    table_meta_client_t(
        mailbox_manager_t *_mailbox_manager,
        watchable_map_t<peer_id_t, table_meta_manager_bcard_t>
            *_table_meta_manager_directory,
        watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_meta_bcard_t>
            *_table_meta_directory);

    /* All of these functions can be called from any thread. */

    /* `find()` determines the ID of the table with the given name in the given database.
    If it finds exactly one such table, sets `*table_id_out` to the table ID and returns
    `true`. Otherwise, sets `*count_out` to the number of tables found and returns
    `false`. `find()` will not block */
    bool find(
        const database_id_t &database,
        const name_string_t &name,
        namespace_id_t *table_id_out,
        size_t *count_out);

    /* `get_name()` determines the name and database of the table with the given ID; it's
    the reverse of `find()`. It returns `false` if there is no visible existing table
    with that ID. `get_name()` will not block. */
    bool get_name(
        const namespace_id_t &table_id,
        database_id_t *db_out,
        name_string_t *name_out);

    /* `list_names()` determines the IDs, names, and databases of every visible table. It
    will not block. */
    void list_names(
        std::map<namespace_id_t, std::pair<database_id_t, name_string_t> > *names_out);

    /* `get_config()` fetches the configuration of the table with the given ID. It may
    block. If it returns `false`, the configuration was not fetched. If it returns
    `true`, `*config_out` will contain the configuration. */
    bool get_config(
        const namespace_id_t &table_id,
        signal_t *interruptor,
        table_config_t *config_out);

    /* `list_configs()` fetches the configurations of every visible table at once. It may
    block. */
    void list_configs(
        signal_t *interruptor,
        std::map<namespace_id_t, table_config_t> *configs_out);

    /* `create()` creates a table with the given configuration. It sets `*table_id_out`
    to the ID of the newly generated table. It may block. If it returns `success`, the
    change will be visible in `find()`, etc. */
    result_t create(
        const table_config_t &new_config,
        signal_t *interruptor,
        namespace_id_t *table_id_out);

    /* `drop()` drops the table with the given ID. It may block. If it returns `false`,
    the change may or may not have succeeded. If it returns `success`, the change will be
    visible in `find()`, etc. */
    result_t drop(
        const namespace_id_t &table_id,
        signal_t *interruptor);

    /* `set_config()` changes the configuration of the table with the given ID. It may
    block. If it returns `false`, the change may or may not have succeeded. If it returns
    `success`, the change will be visible in `find()`, etc. */
    result_t set_config(
        const namespace_id_t &table_id,
        const table_config_t &new_config,
        signal_t *interruptor);

private:
    class table_metadata_t {
    public:
        table_meta_manager_bcard_t::timestamp_t timestamp;
        database_id_t database;
        name_string_t name;
    };

    mailbox_manager_t *mailbox_manager;
    watchable_map_t<peer_id_t, table_meta_manager_bcard_t>
        *table_meta_manager_directory;
    watchable_map_t<std::pair<peer_id_t, namespace_id_t>, table_meta_bcard_t>
        *table_meta_directory;

    watchable_map_var_t<namespace_id_t, table_metadata_t>
        table_metadata_by_id_var;
    all_thread_watchable_map_var_t<namespace_id_t, table_metadata_t>
        table_metadata_by_id;

    watchable_map_var_t<std::pair<database_id_t, name_string_t>,
                        std::set<namespace_id_t> >
        table_ids_by_name_var;
    all_thread_watchable_map_var_t<std::pair<database_id_t, name_string_t>,
                                   std::set<namespace_id_t> >
        table_ids_by_name;
    
};

#endif /* CLUSTERING_TABLE_MANAGER_TABLE_META_CLIENT_HPP_ */

