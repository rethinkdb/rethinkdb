// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_manager/table_metadata.hpp"

#include "clustering/administration/datum_adapter.hpp"

RDB_IMPL_SERIALIZABLE_2_SINCE_v2_1(
    multi_table_manager_timestamp_t::epoch_t, timestamp, id);
RDB_IMPL_SERIALIZABLE_2_SINCE_v2_1(
    multi_table_manager_timestamp_t, epoch, log_index);
RDB_IMPL_SERIALIZABLE_3_FOR_CLUSTER(
    multi_table_manager_bcard_t,
    action_mailbox, get_status_mailbox, server_id);
RDB_IMPL_SERIALIZABLE_3_FOR_CLUSTER(
    table_manager_bcard_t::leader_bcard_t,
    uuid, set_config_mailbox, contract_ack_minidir_bcard);
RDB_IMPL_SERIALIZABLE_6_FOR_CLUSTER(
    table_manager_bcard_t,
    leader, timestamp, raft_member_id, raft_business_card,
    execution_bcard_minidir_bcard, server_id);
RDB_IMPL_SERIALIZABLE_7_FOR_CLUSTER(table_status_request_t,
    want_config, want_sindexes, want_raft_state, want_contract_acks, want_shard_status,
    want_all_replicas_ready, all_replicas_ready_mode);
RDB_IMPL_SERIALIZABLE_7_FOR_CLUSTER(table_status_response_t,
    config, sindexes, raft_state, raft_state_timestamp, contract_acks, shard_status,
    all_replicas_ready);

RDB_IMPL_SERIALIZABLE_2_SINCE_v2_1(table_active_persistent_state_t,
    epoch, raft_member_id);
RDB_IMPL_SERIALIZABLE_2_SINCE_v2_1(table_inactive_persistent_state_t,
    second_hand_config, timestamp);

const microtime_t multi_table_manager_timestamp_t::epoch_t::special_timestamp =
    static_cast<microtime_t>(std::numeric_limits<time_t>::min());

multi_table_manager_timestamp_t::epoch_t
multi_table_manager_timestamp_t::epoch_t::min() {
    epoch_t e;
    e.id = nil_uuid();
    e.timestamp = 0;
    return e;
}

multi_table_manager_timestamp_t::epoch_t
multi_table_manager_timestamp_t::epoch_t::deletion() {
    epoch_t e;
    e.id = nil_uuid();
    e.timestamp = std::numeric_limits<microtime_t>::max();
    return e;
}

multi_table_manager_timestamp_t::epoch_t
multi_table_manager_timestamp_t::epoch_t::make(const epoch_t &prev) {
    guarantee(prev.timestamp != std::numeric_limits<microtime_t>::max());
    microtime_t old_timestamp =
        (prev.timestamp == special_timestamp ? 0 : prev.timestamp);

    epoch_t e;
    e.id = generate_uuid();
    e.timestamp = std::max(current_microtime(), old_timestamp + 1);
    return e;
}

multi_table_manager_timestamp_t::epoch_t
multi_table_manager_timestamp_t::epoch_t::migrate(time_t ts) {
    epoch_t e;
    e.id = generate_uuid();
    e.timestamp = static_cast<microtime_t>(ts < 0 ? 0 : ts);
    return e;
}

ql::datum_t multi_table_manager_timestamp_t::epoch_t::to_datum() const {
    ql::datum_object_builder_t builder;
    builder.overwrite(
        "timestamp", ql::datum_t(static_cast<double>(timestamp)));
    builder.overwrite("id", convert_uuid_to_datum(id));
    return std::move(builder).to_datum();
}

bool multi_table_manager_timestamp_t::epoch_t::is_unset() const {
    return id.is_unset();
}

bool multi_table_manager_timestamp_t::epoch_t::is_deletion() const {
    return id.is_nil() && timestamp == std::numeric_limits<microtime_t>::max();
}

bool multi_table_manager_timestamp_t::epoch_t::operator==(const epoch_t &other) const {
    return timestamp == other.timestamp && id == other.id;
}
bool multi_table_manager_timestamp_t::epoch_t::operator!=(const epoch_t &other) const {
    return !(*this == other);
}

bool multi_table_manager_timestamp_t::epoch_t::supersedes(const epoch_t &other) const {
    // Workaround for issue #4668 - handle invalid timestamps from migration
    microtime_t this_timestamp =
        (timestamp == special_timestamp ? 0 : timestamp);
    microtime_t other_timestamp =
        (other.timestamp == special_timestamp ? 0 : other.timestamp);

    if (this_timestamp > other_timestamp) {
        return true;
    } else if (this_timestamp < other_timestamp) {
        return false;
    } else {
        return other.id < id;
    }
}
