// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_MANAGER_TABLE_METADATA_HPP_
#define CLUSTERING_TABLE_MANAGER_TABLE_METADATA_HPP_

#include "errors.hpp"
#include <boost/optional.hpp>

#include "clustering/administration/persist/file.hpp"
#include "clustering/generic/minidir.hpp"
#include "clustering/generic/raft_core.hpp"
#include "clustering/generic/raft_network.hpp"
#include "clustering/table_contract/contract_metadata.hpp"
#include "clustering/table_contract/cpu_sharding.hpp"
#include "clustering/table_contract/executor/exec.hpp"
#include "rpc/mailbox/typed.hpp"

/* Every message to the `action_mailbox` has an `multi_table_manager_timestamp_t`
attached. This is used to filter out outdated instructions. */
class multi_table_manager_timestamp_t {
public:
    class epoch_t {
    public:
        static epoch_t min();
        static epoch_t deletion();
        static epoch_t migrate(time_t ts);
        static epoch_t make(const epoch_t &prev);

        ql::datum_t to_datum() const;

        bool is_unset() const;
        bool is_deletion() const;
        bool operator==(const epoch_t &other) const;
        bool operator!=(const epoch_t &other) const;
        bool supersedes(const epoch_t &other) const;
    private:
        // Workaround for issue #4668 - invalid timestamps that were migrated
        // These timestamps should never supercede any other timestamps
        static const microtime_t special_timestamp;

        /* Every table's lifetime is divided into "epochs". Each epoch corresponds to
        one Raft instance. Normally tables only have one epoch; a new epoch is created
        only when the user manually overrides the Raft state, which requires creating a
        new Raft instance.

        `timestamp` is the wall-clock time when the epoch began. `id` is a unique ID
        created for the epoch. An epoch with a later `timestamp` supersedes an epoch
        with an earlier `timestamp`. `id` breaks ties. Ties are possible because the
        user may manually override the Raft state on both sides of a netsplit, for
        example. */
        microtime_t timestamp;
        uuid_u id;

        RDB_DECLARE_ME_SERIALIZABLE(epoch_t);
    };

    static multi_table_manager_timestamp_t min() {
        multi_table_manager_timestamp_t ts;
        ts.epoch = epoch_t::min();
        ts.log_index = 0;
        return ts;
    }

    static multi_table_manager_timestamp_t deletion() {
        multi_table_manager_timestamp_t ts;
        ts.epoch = epoch_t::deletion();
        ts.log_index = std::numeric_limits<raft_log_index_t>::max();
        return ts;
    }

    bool is_deletion() const {
        return epoch.is_deletion();
    }

    bool operator==(const multi_table_manager_timestamp_t &other) const {
        return epoch == other.epoch && log_index == other.log_index;
    }
    bool operator!=(const multi_table_manager_timestamp_t &other) const {
        return !(*this == other);
    }

    bool supersedes(const multi_table_manager_timestamp_t &other) const {
        if (epoch.supersedes(other.epoch)) {
            return true;
        } else if (other.epoch.supersedes(epoch)) {
            return false;
        }
        return log_index > other.log_index;
    }

    // TODO: make the data members private and strictly control how they may be changed
    epoch_t epoch;

    /* Within each epoch, Raft log indices provide a monotonically increasing clock. */
    raft_log_index_t log_index;
};
RDB_DECLARE_SERIALIZABLE(multi_table_manager_timestamp_t);

/* In VERIFIED mode, the all replicas ready check makes sure that the leader
still has a quorum and can perform Raft transactions. This is relatively expensive
and causes disk writes and network overhead.
OUTDATED_OK skips that step, but might temporarily return an incorrect result. */
enum class all_replicas_ready_mode_t { INCLUDE_RAFT_TEST = 0, EXCLUDE_RAFT_TEST };
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(all_replicas_ready_mode_t,
                                      int8_t,
                                      all_replicas_ready_mode_t::INCLUDE_RAFT_TEST,
                                      all_replicas_ready_mode_t::EXCLUDE_RAFT_TEST);

class table_status_request_t {
public:
    table_status_request_t() :
        want_config(false), want_sindexes(false), want_raft_state(false),
        want_contract_acks(false), want_shard_status(false),
        want_all_replicas_ready(false),
        all_replicas_ready_mode(all_replicas_ready_mode_t::INCLUDE_RAFT_TEST) { }

    bool want_config;
    bool want_sindexes;
    bool want_raft_state;
    bool want_contract_acks;
    bool want_shard_status;
    bool want_all_replicas_ready;
    all_replicas_ready_mode_t all_replicas_ready_mode;
};
RDB_DECLARE_SERIALIZABLE(table_status_request_t);

class table_status_response_t {
public:
    /* The booleans in `table_status_request_t` control whether each field of
    `table_status_response_t` will be included or not. This is to avoid making an
    expensive computation if the result will not be used. If a field is not requested,
    its value is undefined (but typically empty or default-constructed). */

    /* We must default-initialize boolean fields or they might cause out of range
    errors during serialization and/or deserializtion. */
    table_status_response_t() : all_replicas_ready(false) { }

    /* `config` is controlled by `want_config`. */
    boost::optional<table_config_and_shards_t> config;

    /* `sindexes` is controlled by `want_sindexes`. */
    std::map<std::string, std::pair<sindex_config_t, sindex_status_t> > sindexes;

    /* `raft_state` and `raft_state_timestamp` are controlled by `want_raft_state` */
    boost::optional<table_raft_state_t> raft_state;
    boost::optional<multi_table_manager_timestamp_t> raft_state_timestamp;

    /* `contract_acks` is controlled by `want_contract_acks` */
    std::map<contract_id_t, contract_ack_t> contract_acks;

    /* `shard_status` is controlled by `want_shard_status`. */
    range_map_t<key_range_t::right_bound_t, table_shard_status_t> shard_status;

    /* `all_replicas_ready` is controlled by `want_all_replicas_ready`. It will be set to
    `true` if the responding server is leader and can confirm that all backfills are
    completed, the status matches the config, etc. Otherwise it will be set to `false`.
    */
    bool all_replicas_ready;
};
RDB_DECLARE_SERIALIZABLE(table_status_response_t);

class multi_table_manager_bcard_t {
public:
    enum class status_t { ACTIVE, INACTIVE, DELETED, MAYBE_ACTIVE };

    /* `action_mailbox` handles table creation and deletion, adding and removing servers
    from the table, and manually overriding the table's configuration.

    The fields `status`, `basic_config`, `member_id`, and `initial_state` describe what
    the state of the receiver is supposed to be. `timestamp` is a timestamp as of which
    the information is accurate.

    The possibilities are as follows:
    - `ACTIVE` means that the receiver is supposed to be hosting the table. `member_id`
        and `initial_state` will contain the details. `basic_config` will be empty.
    - `INACTIVE` means the receiver is not supposed to be hosting the table.
        `basic_config` will be present but `member_id` and `initial_state` will be empty.
    - `DELETED` means that the table has been deleted. The other three will be empty.
    - `MAYBE_ACTIVE` means the sender doesn't know if the receiver is supposed to be
        hosting the table or not. `basic_config` will be present but `member_id` and
        `initial_state` will be empty. */
    typedef mailbox_t<void(
        namespace_id_t table_id,
        multi_table_manager_timestamp_t timestamp,
        status_t status,
        boost::optional<table_basic_config_t> basic_config,
        boost::optional<raft_member_id_t> raft_member_id,
        boost::optional<raft_persistent_state_t<table_raft_state_t> > initial_raft_state,
        boost::optional<raft_start_election_immediately_t> start_election_immediately,
        mailbox_t<void()>::address_t ack_addr
        )> action_mailbox_t;
    action_mailbox_t::address_t action_mailbox;

    /* `get_status_mailbox` retrieves configurations, current statuses, etc. for one or
    more tables. Many different types of status queries are combined into one mailbox in
    order to allow more code to be re-used. */
    typedef mailbox_t<void(
        std::set<namespace_id_t> table_ids,
        table_status_request_t request,
        mailbox_t<void(
            std::map<namespace_id_t, table_status_response_t>
            )>::address_t reply_addr
        )> get_status_mailbox_t;
    get_status_mailbox_t::address_t get_status_mailbox;

    /* The server ID of the server sending this business card. In theory you could figure
    it out from the peer ID, but this is way more convenient. Proxy servers will set this
    to `nil_uuid()`. */
    server_id_t server_id;
};
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
    multi_table_manager_bcard_t::status_t,
    int8_t,
    multi_table_manager_bcard_t::status_t::ACTIVE,
    multi_table_manager_bcard_t::status_t::MAYBE_ACTIVE);
RDB_DECLARE_SERIALIZABLE(multi_table_manager_bcard_t);

class table_manager_bcard_t {
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
            table_config_and_shards_change_t config_and_shards_change,
            mailbox_t<void(boost::optional<multi_table_manager_timestamp_t>, bool
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
    - The server has entered a new epoch for the table, or
    - The server has entered or left the Raft cluster, or */
    multi_table_manager_timestamp_t timestamp;

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
RDB_DECLARE_SERIALIZABLE(table_manager_bcard_t::leader_bcard_t);
RDB_DECLARE_SERIALIZABLE(table_manager_bcard_t);

/* If we are an active member for a given table, we'll store a
`table_active_persistent_state_t` plus a `raft_persistent_state_t<table_raft_state_t>`;
the latter will be stored as individual components for better efficiency. If we are not
an active member, we'll store a `table_inactive_persistent_state_t`. If the table is
deleted we won't store anything. */

class table_active_persistent_state_t {
public:
    multi_table_manager_timestamp_t::epoch_t epoch;
    raft_member_id_t raft_member_id;
};

RDB_DECLARE_SERIALIZABLE(table_active_persistent_state_t);

class table_inactive_persistent_state_t {
public:
    table_basic_config_t second_hand_config;

    /* `timestamp` records a time at which `second_hand_config` is known to have been
    correct. */
    multi_table_manager_timestamp_t timestamp;
};

RDB_DECLARE_SERIALIZABLE(table_inactive_persistent_state_t);

class table_persistence_interface_t {
public:
    /* Some of these methods return a `raft_storage_interface_t *`. This returned pointer
    will remain valid until the next call to `read_all_metadata()`, `write_metadata_*()`
    or `delete_metadata()` affecting that table. */

    /* Finds all tables stored in the metadata and calls the appropriate callback. Note
    that this invalidates any existing `raft_storage_interface_t`s! */
    virtual void read_all_metadata(
        const std::function<void(
            const namespace_id_t &table_id,
            const table_active_persistent_state_t &state,
            raft_storage_interface_t<table_raft_state_t> *raft_storage,
            metadata_file_t::read_txn_t *metadata_read_txn)> &active_cb,
        const std::function<void(
            const namespace_id_t &table_id,
            const table_inactive_persistent_state_t &state,
            metadata_file_t::read_txn_t *metadata_read_txn)> &inactive_cb,
        signal_t *interruptor) = 0;

    /* `write_metadata_active()` sets the stored metadata for the table to be the given
    `state` and `raft_state`. It then returns a `raft_storage_interface_t *` that can be
    used to make incremental updates to the Raft state. Any previously existing
    `raft_storage_interface_t *` for this table will be invalidated. */
    virtual void write_metadata_active(
        const namespace_id_t &table_id,
        const table_active_persistent_state_t &state,
        const raft_persistent_state_t<table_raft_state_t> &raft_state,
        raft_storage_interface_t<table_raft_state_t> **raft_storage_out) = 0;

    /* `write_metadata_inactive()` sets the stored metadata for the table to be the given
    `state`, which is inactive. */
    virtual void write_metadata_inactive(
        const namespace_id_t &table_id,
        const table_inactive_persistent_state_t &state) = 0;

    /* `delete_metadata()` deletes all metadata for the table. */
    virtual void delete_metadata(
        const namespace_id_t &table_id) = 0;

    virtual void load_multistore(
        const namespace_id_t &table_id,
        metadata_file_t::read_txn_t *metadata_read_txn,
        scoped_ptr_t<multistore_ptr_t> *multistore_ptr_out,
        signal_t *interruptor,
        perfmon_collection_t *perfmon_collection_serializers) = 0;
    virtual void create_multistore(
        const namespace_id_t &table_id,
        scoped_ptr_t<multistore_ptr_t> *multistore_ptr_out,
        signal_t *interruptor,
        perfmon_collection_t *perfmon_collection_serializers) = 0;
    virtual void destroy_multistore(
        const namespace_id_t &table_id,
        scoped_ptr_t<multistore_ptr_t> *multistore_ptr_in) = 0;

protected:
    virtual ~table_persistence_interface_t() { }
};

#endif /* CLUSTERING_TABLE_MANAGER_TABLE_METADATA_HPP_ */

