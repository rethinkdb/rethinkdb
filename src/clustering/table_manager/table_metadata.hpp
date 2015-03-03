// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_MANAGER_TABLE_METADATA_HPP_
#define CLUSTERING_TABLE_MANAGER_TABLE_METADATA_HPP_

#include "clustering/generic/minidir.hpp"
#include "clustering/generic/raft_core.hpp"
#include "clustering/generic/raft_network.hpp"
#include "clustering/table_contract/contract_metadata.hpp"
#include "clustering/table_contract/exec.hpp"
#include "rpc/mailbox/typed.hpp"

class table_meta_manager_bcard_t {
public:
    /* Every message to the `action_mailbox` has an `timestamp_t` attached. This is used
    to filter out outdated instructions. */
    class timestamp_t {
    public:
        class epoch_t {
        public:
            /* Every table's lifetime is divided into "epochs". Each epoch corresponds to
            one Raft instance. Normally tables only have one epoch; a new epoch is
            created only when the user manually overrides the Raft state, which requires
            creating a new Raft instance.

            `timestamp` is the wall-clock time when the epoch began. `id` is a unique ID
            created for the epoch. An epoch with a later `timestamp` supersedes an epoch
            with an earlier `timestamp`. `id` breaks ties. Ties are possible because the
            user may manually override the Raft state on both sides of a netsplit, for
            example. */
            microtime_t timestamp;
            uuid_u id;

            bool operator==(const epoch_t &other) const {
                return timestamp == other.timestamp && id == other.id;
            }
            bool operator!=(const epoch_t &other) const {
                return !(*this == other);
            }

            bool supersedes(const epoch_t &other) const {
                if (timestamp > other.timestamp) {
                    return true;
                } else if (timestamp < other.timestamp) {
                    return false;
                } else {
                    return other.id < id;
                }
            }
        };

        epoch_t epoch;

        /* Within each epoch, Raft log indices provide a monotonically increasing clock.
        */
        raft_log_index_t log_index;

        bool supersedes(const timestamp_t &other) const {
            if (epoch.supersedes(other.epoch)) {
                return true;
            } else if (other.epoch.supersedes(epoch)) {
                return false;
            }
            return log_index > other.log_index;
        }
    };

    /* `action_mailbox` handles table creation and deletion, adding and removing servers
    from the table, and manually overriding the table's configuration. */
    typedef mailbox_t<void(
        namespace_id_t table_id,
        timestamp_t timestamp,
        bool is_deletion,
        boost::optional<raft_member_id_t> member_id,
        boost::optional<raft_persistent_state_t<table_raft_state_t> > initial_state,
        mailbox_t<void()>::address_t ack_addr
        )> action_mailbox_t;
    action_mailbox_t::address_t action_mailbox;

    /* `get_config_mailbox` handles fetching the current value of the
    `table_config_and_shards_t` for a specific table or all tables. If `table_id` is
    non-empty, the receiver will reply with a map with zero or one entries, depending on
    if it is hosting the given table or not. If `table_id` is empty, the receiver will
    reply with an entry for every table it is hosting. */
    typedef mailbox_t<void(
        boost::optional<namespace_id_t> table_id,
        mailbox_t<void(std::map<namespace_id_t, table_config_and_shards_t>)>::
            address_t reply_addr
        )> get_config_mailbox_t;
    get_config_mailbox_t::address_t get_config_mailbox;

    /* The server ID of the server sending this business card. In theory you could figure
    it out from the peer ID, but this is way more convenient. */
    server_id_t server_id;
};
RDB_DECLARE_SERIALIZABLE(
    table_meta_manager_bcard_t::timestamp_t::epoch_t);
RDB_DECLARE_SERIALIZABLE(table_meta_manager_bcard_t::timestamp_t);
RDB_DECLARE_SERIALIZABLE(table_meta_manager_bcard_t);

class table_meta_bcard_t {
public:
    /* Whichever server is Raft leader will publish a `leader_bcard_t` in its directory.
    This is the destination for config change messages, and also the way that contract
    acks find their way to the contract coordinator. */
    class leader_bcard_t {
    public:
        /* Sometimes a server may stop being leader and then start again in quick
        succession. It will have a new set of mailboxes, and so any messages that were in
        flight to the old set of mailboxes will be dropped. Message senders need an easy
        way to detect when this happens. The solution is the `uuid` field; every set of
        mailboxes will get a unique UUID, so if the UUID changes, senders know they need
        to re-send their messages. */
        uuid_u uuid;

        /* `set_config_mailbox` handles changes to the `table_config_and_shards_t`. These
        changes may or may not involve adding and removing servers; if they do, then the
        initial config change message will trigger subsequent action messages to add and
        remove the servers. If the change was committed, it returns the action timestamp
        for the commit; the client can use this to determine which servers have seen the
        commit. If something goes wrong, it returns an empty `boost::optional`, in which
        case the change may or may not eventually be committed. */
        typedef mailbox_t<void(
            table_config_and_shards_t new_config_and_shards,
            mailbox_t<void(boost::optional<table_meta_manager_bcard_t::timestamp_t>
                )>::address_t reply_addr
            )> set_config_mailbox_t;
        set_config_mailbox_t::address_t set_config_mailbox;

        /* `contract_executor_t`s for this table on other servers send contract acks to
        the `contract_coordinator_t` for this table via this bcard. */
        minidir_bcard_t<std::pair<server_id_t, contract_id_t>, contract_ack_t>
            contract_ack_minidir_bcard;
    };
    boost::optional<leader_bcard_t> leader;

    /* This timestamp contains a `raft_log_index_t`. It would be expensive to update the
    directory every time a Raft commit happened. Therefore, this timestamp is only
    guaranteed to be updated when:
    - The server has entered a new epoch for the table, or;
    - The server has entered or left the Raft cluster, or;
    - The table's name or database have changed. */
    table_meta_manager_bcard_t::timestamp_t timestamp;

    /* `database` and `name` are the table's database and name. They are distributed in
    the directory so that every server can efficiently look up tables by name. */
    database_id_t database;
    name_string_t name;

    /* The table's primary key. This is distributed in the directory so that every server
    can efficiently run queries that require knowing the primary key. */
    std::string primary_key;

    /* The other members of the Raft cluster send Raft RPCs through
    `raft_business_card`. */
    raft_member_id_t raft_member_id;
    raft_business_card_t<table_raft_state_t> raft_business_card;

    /* `contract_executor_t`s for this table on other servers send messages to the
    `contract_executor_t` on this server via this minidir bcard. */
    minidir_bcard_t<std::pair<server_id_t, branch_id_t>, contract_execution_bcard_t>
        execution_bcard_minidir_bcard;

    /* The server ID of the server sending this business card. In theory you could figure
    it out from the peer ID, but this is way more convenient. */
    server_id_t server_id;
};
RDB_DECLARE_SERIALIZABLE(table_meta_bcard_t::leader_bcard_t);
RDB_DECLARE_SERIALIZABLE(table_meta_bcard_t);

#endif /* CLUSTERING_TABLE_MANAGER_TABLE_METADATA_HPP_ */

