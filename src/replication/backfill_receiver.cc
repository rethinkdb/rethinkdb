#include "replication/backfill_receiver.hpp"

namespace replication {

perfmon_duration_sampler_t
    pm_replication_slave_handling_2("replication_slave_handling_2", secs_to_ticks(1.0));

void backfill_receiver_t::send(scoped_malloc<net_backfill_complete_t>& message) {
    block_pm_duration timer(&pm_replication_slave_handling_2);
    order_token_t token = order_source->check_in_backfill_operation("net_backfill_complete_t");
    order_source->backfill_done();
    cb->backfill_done(message->time_barrier_timestamp, token);
}

void backfill_receiver_t::send(scoped_malloc<net_get_cas_t>& msg) {
    block_pm_duration timer(&pm_replication_slave_handling_2);
    order_token_t token = order_source->check_in_realtime_operation("net_get_cas_t");
    store_key_t key(msg->key_size, msg->key);
    castime_t castime(msg->proposed_cas, msg->timestamp);
    cb->realtime_get_cas(key, castime, token);
}

void backfill_receiver_t::send(stream_pair<net_sarc_t>& msg) {
    block_pm_duration timer(&pm_replication_slave_handling_2);
    order_token_t token = order_source->check_in_realtime_operation("net_sarc_t");
    store_key_t key(msg->key_size, msg->keyvalue);
    castime_t castime(msg->proposed_cas, msg->timestamp);
    cb->realtime_sarc(key, msg.stream, msg->flags, msg->exptime, castime,
                      add_policy_t(msg->add_policy), replace_policy_t(msg->replace_policy),
                      msg->old_cas, token);
}

void backfill_receiver_t::send(stream_pair<net_backfill_set_t>& msg) {
    block_pm_duration timer(&pm_replication_slave_handling_2);
    order_token_t token = order_source->check_in_backfill_operation("net_backfill_set_t");
    backfill_atom_t atom;
    atom.key.assign(msg->key_size, msg->keyvalue);
    atom.value = msg.stream;
    atom.flags = msg->flags;
    atom.exptime = msg->exptime;
    atom.recency = msg->timestamp;
    atom.cas_or_zero = msg->cas_or_zero;
    cb->backfill_set(atom, token);
}

void backfill_receiver_t::send(scoped_malloc<net_incr_t>& msg) {
    block_pm_duration timer(&pm_replication_slave_handling_2);
    order_token_t token = order_source->check_in_realtime_operation("net_incr_t");
    store_key_t key(msg->key_size, msg->key);
    castime_t castime(msg->proposed_cas, msg->timestamp);
    cb->realtime_incr_decr(incr_decr_INCR, key, msg->amount, castime, token);
}

void backfill_receiver_t::send(scoped_malloc<net_decr_t>& msg) {
    block_pm_duration timer(&pm_replication_slave_handling_2);
    order_token_t token = order_source->check_in_realtime_operation("net_decr_t");
    store_key_t key(msg->key_size, msg->key);
    castime_t castime(msg->proposed_cas, msg->timestamp);
    cb->realtime_incr_decr(incr_decr_DECR, key, msg->amount, castime, token);
}

void backfill_receiver_t::send(stream_pair<net_append_t>& msg) {
    block_pm_duration timer(&pm_replication_slave_handling_2);
    order_token_t token = order_source->check_in_realtime_operation("net_append_t");
    store_key_t key(msg->key_size, msg->keyvalue);
    castime_t castime(msg->proposed_cas, msg->timestamp);
    cb->realtime_append_prepend(append_prepend_APPEND, key, msg.stream, castime, token);
}

void backfill_receiver_t::send(stream_pair<net_prepend_t>& msg) {
    block_pm_duration timer(&pm_replication_slave_handling_2);
    order_token_t token = order_source->check_in_realtime_operation("net_prepend_t");
    store_key_t key(msg->key_size, msg->keyvalue);
    castime_t castime(msg->proposed_cas, msg->timestamp);
    cb->realtime_append_prepend(append_prepend_PREPEND, key, msg.stream, castime, token);
}

void backfill_receiver_t::send(scoped_malloc<net_delete_t>& msg) {
    block_pm_duration timer(&pm_replication_slave_handling_2);
    order_token_t token = order_source->check_in_realtime_operation("net_delete_t");
    store_key_t key(msg->key_size, msg->key);
    cb->realtime_delete_key(key, msg->timestamp, token);
}

void backfill_receiver_t::send(scoped_malloc<net_backfill_delete_t>& msg) {
    block_pm_duration timer(&pm_replication_slave_handling_2);
    order_token_t token = order_source->check_in_backfill_operation("net_backfill_delete_t");
    store_key_t key(msg->key_size, msg->key);
    cb->backfill_deletion(key, token);
}

void backfill_receiver_t::send(UNUSED scoped_malloc<net_heartbeat_t>& msg) {
    block_pm_duration timer(&pm_replication_slave_handling_2);
    // Ignore the heartbeat, it has been handled in the protocol layer already.
}

void backfill_receiver_t::timebarrier_helper(net_timebarrier_t msg) {
    block_pm_duration timer(&pm_replication_slave_handling_2);
    order_token_t token = order_source->check_in_realtime_operation("net_timebarrier_t");
    cb->realtime_time_barrier(msg.timestamp, token);
}

}
