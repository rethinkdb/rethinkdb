#include "replication/debug.hpp"
#include "utils2.hpp"
#include "store.hpp"

namespace replication {

std::string debug_format(const net_hello_t *msg) {
    return strprintf("net_hello_t { replication_protocol_version = %u; }",
        msg->replication_protocol_version);
}

std::string debug_format(const net_introduce_t *msg) {
    return strprintf("net_introduce_t { database_creation_timestamp = %u; other_id = %u; }",
        msg->database_creation_timestamp, msg->other_id);
}

std::string debug_format(const net_backfill_t *msg) {
    return strprintf("net_backfill_t { timestamp = %u; }",
        msg->timestamp.time);
}

std::string debug_format(const net_backfill_complete_t *msg) {
    return strprintf("net_backfill_complete_t { timestamp = %u; }",
        msg->time_barrier_timestamp.time);
}

std::string debug_format(UNUSED const net_backfill_delete_everything_t *msg) {
    return strprintf("net_backfill_delete_everything_t { }");
}

std::string debug_format(const net_nop_t *msg) {
    return strprintf("net_nop_t { timestamp = %u; }",
        msg->timestamp.time);
}

std::string debug_format(const net_get_cas_t *msg) {
    return strprintf("net_backfill_complete_t { proposed_cas = %lu; timestamp = %u; "
            "key = \"%*.*s\"; }",
        msg->proposed_cas, msg->timestamp.time,
        (int)msg->key_size, (int)msg->key_size, msg->key);
}

std::string debug_format(const net_sarc_t *msg) {
    return strprintf("net_sarc_t { timestamp = %u; proposed_cas = %lu; flags = %u; exptime = %u; "
            "add_policy = %s; replace_policy = %s; old_cas = %lu; key = \"%*.*s\"; "
            "value = \"%*.*s\"; }",
        msg->timestamp.time, msg->proposed_cas, msg->flags, msg->exptime,
        (msg->add_policy == add_policy_yes) ? "yes" : "no",
        (msg->replace_policy == replace_policy_yes) ? "yes" :
            (msg->replace_policy == replace_policy_no) ? "no" : "if_cas_matches",
        msg->old_cas,
        (int)msg->key_size, (int)msg->key_size, msg->keyvalue,
        msg->value_size, msg->value_size, msg->keyvalue + msg->key_size);
}

std::string debug_format(const net_backfill_set_t *msg) {
    return strprintf("net_backfill_set_t { timestamp = %u; flags = %u; exptime = %u; "
            "cas_or_zero = %lu; key = \"%*.*s\"; value = \"%*.*s\"; }",
        msg->timestamp.time, msg->flags, msg->exptime, msg->cas_or_zero,
        (int)msg->key_size, (int)msg->key_size, msg->keyvalue,
        msg->value_size, msg->value_size, msg->keyvalue + msg->key_size);
}

std::string debug_format(const net_incr_t *msg) {
    return strprintf("net_incr_t { timestamp = %u; proposed_cas = %lu; amount = %lu; "
            "key = \"%*.*s\"; }",
        msg->timestamp.time, msg->proposed_cas, msg->amount,
        (int)msg->key_size, (int)msg->key_size, msg->key);
}

std::string debug_format(const net_decr_t *msg) {
    return strprintf("net_decr_t { timestamp = %u; proposed_cas = %lu; amount = %lu; "
            "key = \"%*.*s\"; }",
        msg->timestamp.time, msg->proposed_cas, msg->amount,
        (int)msg->key_size, (int)msg->key_size, msg->key);
}

std::string debug_format(const net_append_t *msg) {
    return strprintf("net_append_t { timestamp = %u; proposed_cas = %lu; key = \"%*.*s\"; "
            "value = \"%*.*s\"; }",
        msg->timestamp.time, msg->proposed_cas,
        (int)msg->key_size, (int)msg->key_size, msg->keyvalue,
        msg->value_size, msg->value_size, msg->keyvalue + msg->key_size);
}

std::string debug_format(const net_prepend_t *msg) {
    return strprintf("net_prepend_t { timestamp = %u; proposed_cas = %lu; key = \"%*.*s\"; "
            "value = \"%*.*s\"; }",
        msg->timestamp.time, msg->proposed_cas,
        (int)msg->key_size, (int)msg->key_size, msg->keyvalue,
        msg->value_size, msg->value_size, msg->keyvalue + msg->key_size);
}

std::string debug_format(const net_delete_t *msg) {
    return strprintf("net_delete_t { timestamp = %u; key = \"%*.*s\"; }",
        msg->timestamp.time, (int)msg->key_size, (int)msg->key_size, msg->key);
}

std::string debug_format(const net_backfill_delete_t *msg) {
    return strprintf("net_backfill_delete_t { key = \"%*.*s\"; }",
        (int)msg->key_size, (int)msg->key_size, msg->key);
}

}   // namespace replication
