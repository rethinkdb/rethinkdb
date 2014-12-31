// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_REACTOR_TABLE_MANAGER_HPP_
#define CLUSTERING_REACTOR_TABLE_MANAGER_HPP_

/* Each server's `table_meta_manager_t` creates one `table_manager_t` for each table
hosted on that server. The `table_manager_t` sets up the `reactor_t` that actually
handles queries for the table, and it also handles reconfiguration operations for the
table. However, table creation and deletion, along with any reconfiguration actions that
add or remove serverss from the table, require the help fo the `table_meta_manager_t`. */

class table_business_card_t {
public:
    /* The parsing server uses this mailbox to read the current table configuration from
    the Raft cluster. */
    typedef mailbox_t<void(
        mailbox_t<void(table_config_t)>::address_t
        )> get_config_mailbox_t;
    get_config_mailbox_t::address_t get_config;

    /* This mailbox is for writing the Raft cluster state. Only the Raft leader will
    expose this mailbox; the others will have `set_config` empty. */
    typedef mailbox_t<void(
        table_config_t,
        mailbox_t<void(bool)>::address_t
        )> set_config_mailbox_t;
    boost::optional<set_config_mailbox_t::address_t> set_config;
};

class table_raft_state_t {
public:
    table_config_t config;
    std::map<server_id_t, raft_member_id_t> member_ids;
};

class table_manager_t {
public:
    table_manager_t(
        const server_id_t &_server_id,
        const namespace_id_t &_table_id,
        mailbox_manager_t *_mailbox_manager,
        raft_member_t<table_raft_state_t> *_raft_member,
        /* The following parameters are for the benefit of the `reactor_t` */
        multistore_ptr_t *_multistore_ptr,
        const base_path_t &_base_path,
        io_backender_t *_io_backender,
        backfill_throttler_t *_backfill_throttler,
        branch_history_manager_t *_branch_history_manager,
        perfmon_collection_t *_perfmon_collection,
        rdb_context_t *_rdb_context,
        watchable_map_t<
            std::pair<peer_id_t, namespace_id_t>,
            namespace_directory_metadata_t
            > *_reactor_directory_view);

    clone_ptr_t<watchable_t<table_business_card_t> > get_business_card();
};

#endif /* CLUSTERING_REACTOR_TABLE_MANAGER_HPP_ */

