#include "replication/backfill_receiver.hpp"

namespace replication {

backfill_receiver_t::backfill_receiver_t(btree_key_value_store_t *underlying)
    : backfilling_(false), internal_store_(underlying) { }

backfill_receiver_t::~backfill_receiver_t() {
    if (backfilling_) {
        logWRN("A backfill operation is being interrupted. The data in this database is now "
            "in an inconsistent state. To get the data back into a consistent state, "
            "reestablish the master-slave connection and allow the backfill to run to "
            "completion.\n");
        /* Don't call internal_store_.backfill_complete(), because that would update the timestamp,
        which would prevent us from redoing this backfill later. */
    }
}

void backfill_receiver_t::send(scoped_malloc<net_backfill_complete_t>& message) {
    internal_store_.backfill_complete(message->time_barrier_timestamp);
    backfilling_ = false;
}

void backfill_receiver_t::send(scoped_malloc<net_get_cas_t>& msg) {
    get_cas_mutation_t mut;
    mut.key.assign(msg->key_size, msg->key);
    internal_store_.handover(new mutation_t(mut), castime_t(msg->proposed_cas, msg->timestamp));
}

void backfill_receiver_t::send(stream_pair<net_sarc_t>& msg) {
    sarc_mutation_t mut;
    mut.key.assign(msg->key_size, msg->keyvalue);
    mut.data = msg.stream;
    mut.flags = msg->flags;
    mut.exptime = msg->exptime;
    mut.add_policy = add_policy_t(msg->add_policy);
    mut.replace_policy = replace_policy_t(msg->replace_policy);
    mut.old_cas = msg->old_cas;

    internal_store_.handover(new mutation_t(mut), castime_t(msg->proposed_cas, msg->timestamp));
}

void backfill_receiver_t::send(stream_pair<net_backfill_set_t>& msg) {
    backfilling_ = true;

    sarc_mutation_t mut;
    mut.key.assign(msg->key_size, msg->keyvalue);
    mut.data = msg.stream;
    mut.flags = msg->flags;
    mut.exptime = msg->exptime;
    mut.add_policy = add_policy_yes;
    mut.replace_policy = replace_policy_yes;
    mut.old_cas = NO_CAS_SUPPLIED;

    internal_store_.backfill_handover(new mutation_t(mut), castime_t(msg->cas_or_zero, msg->timestamp));

    if (msg->cas_or_zero != 0) {
        /* We need to make sure that the key gets assigned a CAS, because it has a CAS on the
        other node. If the key did not have a CAS before we sent "mut", then it will still lack a
        CAS after we send "mut", so we also send "cas_mut" to force it to have a CAS. Since we
        send "msg->cas_or_zero" as the "proposed_cas" for "cas_mut", the CAS will be correct. If
        the key had a CAS before we sent "mut", then "mut" will set the CAS to the correct value,
        and "cas_mut" will be a noop. */
        get_cas_mutation_t cas_mut;
        cas_mut.key = mut.key;
        internal_store_.backfill_handover(new mutation_t(cas_mut), castime_t(msg->cas_or_zero, msg->timestamp));
    }
}

void backfill_receiver_t::send(scoped_malloc<net_incr_t>& msg) {
    incr_decr_mutation_t mut;
    mut.key.assign(msg->key_size, msg->key);
    mut.kind = incr_decr_INCR;
    mut.amount = msg->amount;
    internal_store_.handover(new mutation_t(mut), castime_t(msg->proposed_cas, msg->timestamp));
}

void backfill_receiver_t::send(scoped_malloc<net_decr_t>& msg) {
    incr_decr_mutation_t mut;
    mut.key.assign(msg->key_size, msg->key);
    mut.kind = incr_decr_DECR;
    mut.amount = msg->amount;
    internal_store_.handover(new mutation_t(mut), castime_t(msg->proposed_cas, msg->timestamp));
}

void backfill_receiver_t::send(stream_pair<net_append_t>& msg) {
    append_prepend_mutation_t mut;
    mut.key.assign(msg->key_size, msg->keyvalue);
    mut.data = msg.stream;
    mut.kind = append_prepend_APPEND;
    internal_store_.handover(new mutation_t(mut), castime_t(msg->proposed_cas, msg->timestamp));
}

void backfill_receiver_t::send(stream_pair<net_prepend_t>& msg) {
    append_prepend_mutation_t mut;
    mut.key.assign(msg->key_size, msg->keyvalue);
    mut.data = msg.stream;
    mut.kind = append_prepend_PREPEND;
    internal_store_.handover(new mutation_t(mut), castime_t(msg->proposed_cas, msg->timestamp));
}

void backfill_receiver_t::send(scoped_malloc<net_delete_t>& msg) {
    delete_mutation_t mut;
    mut.key.assign(msg->key_size, msg->key);
    // TODO: where does msg->timestamp go???  IS THIS RIGHT?? WHO KNOWS.
    internal_store_.handover(new mutation_t(mut), castime_t(NO_CAS_SUPPLIED /* This isn't even used, why is it a parameter. */, msg->timestamp));
}

void backfill_receiver_t::send(scoped_malloc<net_backfill_delete_t>& msg) {
    backfilling_ = true;

    delete_mutation_t mut;
    mut.key.assign(msg->key_size, msg->key);
    // NO_CAS_SUPPLIED is not used in any way for deletions, and the
    // timestamp is part of the "->change" interface in a way not
    // relevant to slaves -- it's used when putting deletions into the
    // delete queue.
    internal_store_.backfill_handover(new mutation_t(mut), castime_t(NO_CAS_SUPPLIED, repli_timestamp::invalid));
}

void backfill_receiver_t::send(scoped_malloc<net_nop_t>& message) {
    internal_store_.time_barrier(message->timestamp);
}

}
