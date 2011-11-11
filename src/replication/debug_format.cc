#include "replication/debug_format.hpp"
#include "utils.hpp"
#include "memcached/store.hpp"

namespace replication {

std::string debug_format(int count, const char *data) {
    const int max_length = 100;
    if (count < max_length) {
        return strprintf("%d \"%*.*s\"", count, count, count, data);
    } else {
        int chunk = (max_length - 3) / 2;
        return strprintf("%d \"%*.*s...%*.*s\"", count,
            chunk, chunk, data,
            chunk, chunk, data + count - chunk);
    }
}

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

std::string debug_format(const net_timebarrier_t *msg) {
    return strprintf("net_timebarrier_t { timestamp = %u; }",
        msg->timestamp.time);
}

std::string debug_format(UNUSED const net_heartbeat_t *msg) {
    return strprintf("net_heartbeat_t { }");
}

std::string debug_format(const net_get_cas_t *msg) {
    return strprintf("net_backfill_complete_t { proposed_cas = %lu; timestamp = %u; "
            "key = \"%*.*s\"; }",
        msg->proposed_cas, msg->timestamp.time,
        (int)msg->key_size, (int)msg->key_size, msg->key);
}

std::string debug_format(const net_sarc_t *msg, const void *real_data) {
    return strprintf("net_sarc_t { timestamp = %u; proposed_cas = %lu; flags = %u; exptime = %u; "
            "add_policy = %s; replace_policy = %s; old_cas = %lu; key = %s; value = %s; }",
        msg->timestamp.time, msg->proposed_cas, msg->flags, msg->exptime,
        (msg->add_policy == add_policy_yes) ? "yes" : "no",
        (msg->replace_policy == replace_policy_yes) ? "yes" :
            (msg->replace_policy == replace_policy_no) ? "no" : "if_cas_matches",
        msg->old_cas,
        debug_format(msg->key_size, msg->keyvalue).c_str(),
                     debug_format(msg->value_size, real_data ? reinterpret_cast<const char *>(real_data) : msg->keyvalue + msg->key_size).c_str()
        );
}

std::string debug_format(const net_backfill_set_t *msg, const void *real_data) {
    return strprintf("net_backfill_set_t { timestamp = %u; flags = %u; exptime = %u; "
            "cas_or_zero = %lu; key = %s; value = %s; }",
        msg->timestamp.time, msg->flags, msg->exptime, msg->cas_or_zero,
        debug_format(msg->key_size, msg->keyvalue).c_str(),
                     debug_format(msg->value_size, real_data ? reinterpret_cast<const char *>(real_data) : msg->keyvalue + msg->key_size).c_str()
        );
}

std::string debug_format(const net_incr_t *msg) {
    return strprintf("net_incr_t { timestamp = %u; proposed_cas = %lu; amount = %lu; key = %s; }",
        msg->timestamp.time, msg->proposed_cas, msg->amount,
        debug_format(msg->key_size, msg->key).c_str());
}

std::string debug_format(const net_decr_t *msg) {
    return strprintf("net_decr_t { timestamp = %u; proposed_cas = %lu; amount = %lu; key = %s; }",
        msg->timestamp.time, msg->proposed_cas, msg->amount,
        debug_format(msg->key_size, msg->key).c_str());
}

std::string debug_format(const net_append_t *msg, const void *real_data) {
    return strprintf("net_append_t { timestamp = %u; proposed_cas = %lu; key = %s; value = %s; }",
        msg->timestamp.time, msg->proposed_cas,
        debug_format(msg->key_size, msg->keyvalue).c_str(),
                     debug_format(msg->value_size, real_data ? reinterpret_cast<const char *>(real_data) : msg->keyvalue + msg->key_size).c_str()
        );
}

std::string debug_format(const net_prepend_t *msg, const void *real_data) {
    return strprintf("net_prepend_t { timestamp = %u; proposed_cas = %lu; key = %s; value = %s; }",
        msg->timestamp.time, msg->proposed_cas,
        debug_format(msg->key_size, msg->keyvalue).c_str(),
                     debug_format(msg->value_size, real_data ? reinterpret_cast<const char *>(real_data) : msg->keyvalue + msg->key_size).c_str()
        );
}

std::string debug_format(const net_delete_t *msg) {
    return strprintf("net_delete_t { timestamp = %u; key = %s; }",
        msg->timestamp.time, debug_format(msg->key_size, msg->key).c_str());
}

std::string debug_format(const net_backfill_delete_range_t *msg) {
    bool low_key_supplied = msg->low_key_size != net_backfill_delete_range_t::infinity_key_size;
    bool high_key_supplied = msg->high_key_size != net_backfill_delete_range_t::infinity_key_size;
    int high_key_offset = low_key_supplied ? msg->low_key_size : 0;

    return strprintf("net_backfill_delete_range_t { hash_value = %u; hashmod = %u; low_key = %s; high_key = %s; }",
                     msg->hash_value, msg->hashmod,
                     low_key_supplied ? debug_format(msg->low_key_size, msg->keys).c_str() : "[missing]",
                     high_key_supplied ? debug_format(msg->high_key_size, msg->keys + high_key_offset).c_str() : "[missing]");
}

std::string debug_format(const net_backfill_delete_t *msg) {
    return strprintf("net_backfill_delete_t { timestamp = %u; key = %s; }",
                     msg->timestamp.time, debug_format(msg->key_size, msg->key).c_str());
}

}   // namespace replication
