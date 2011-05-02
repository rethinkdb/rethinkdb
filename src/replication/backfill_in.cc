#include "replication/backfill_in.hpp"

perfmon_duration_sampler_t
    pm_slave_sarc("slave_sarc", secs_to_ticks(1.0)),
    pm_slave_del("slave_del", secs_to_ticks(1.0)),
    pm_slave_rt_get_cas("slave_rt_get_cas", secs_to_ticks(1.0)),
    pm_slave_rt_sarc("slave_rt_sarc", secs_to_ticks(1.0)),
    pm_slave_rt_incr_decr("slave_rt_incr_decr", secs_to_ticks(1.0)),
    pm_slave_rt_app_prep("slave_rt_app_prep", secs_to_ticks(1.0)),
    pm_slave_rt_del("slave_rt_del", secs_to_ticks(1.0));

namespace replication {

backfill_storer_t::backfill_storer_t(btree_key_value_store_t *underlying)
    : backfilling_(false), internal_store_(underlying) { }

backfill_storer_t::~backfill_storer_t() {
    if (backfilling_) {
        logWRN("A backfill operation is being interrupted. The data in this database is now "
            "in an inconsistent state. To get the data back into a consistent state, "
            "reestablish the master-slave connection and allow the backfill to run to "
            "completion.\n");
        /* Don't call internal_store_.backfill_complete(), because that would update the timestamp,
        which would prevent us from redoing this backfill later. */
    }
}

void backfill_storer_t::backfill_delete_everything() {
    backfilling_ = true;
    internal_store_.backfill_delete_all_keys();
}

void backfill_storer_t::backfill_deletion(store_key_t key) {
    block_pm_duration set_timer(&pm_slave_del);
    backfilling_ = true;

    delete_mutation_t mut;
    mut.key = key;
    mut.dont_put_in_delete_queue = true;
    // NO_CAS_SUPPLIED is not used in any way for deletions, and the
    // timestamp is part of the "->change" interface in a way not
    // relevant to slaves -- it's used when putting deletions into the
    // delete queue.
    internal_store_.backfill_handover(new mutation_t(mut), castime_t(NO_CAS_SUPPLIED, repli_timestamp::invalid));
}

void backfill_storer_t::backfill_set(backfill_atom_t atom) {
    block_pm_duration set_timer(&pm_slave_sarc);
    backfilling_ = true;

    sarc_mutation_t mut;
    mut.key = atom.key;
    mut.data = atom.value;
    mut.flags = atom.flags;
    mut.exptime = atom.exptime;
    mut.add_policy = add_policy_yes;
    mut.replace_policy = replace_policy_yes;
    mut.old_cas = NO_CAS_SUPPLIED;

    internal_store_.backfill_handover(new mutation_t(mut), castime_t(atom.cas_or_zero, atom.recency));

    if (atom.cas_or_zero != 0) {
        /* We need to make sure that the key gets assigned a CAS, because it has a CAS on the
        other node. If the key did not have a CAS before we sent "mut", then it will still lack a
        CAS after we send "mut", so we also send "cas_mut" to force it to have a CAS. Since we
        send "atom.cas_or_zero" as the "proposed_cas" for "cas_mut", the CAS will be correct. If
        the key had a CAS before we sent "mut", then "mut" will set the CAS to the correct value,
        and "cas_mut" will be a noop. */
        get_cas_mutation_t cas_mut;
        cas_mut.key = mut.key;
        internal_store_.backfill_handover(new mutation_t(cas_mut), castime_t(atom.cas_or_zero, atom.recency));
    }
}

void backfill_storer_t::backfill_done(repli_timestamp_t timestamp) {
    internal_store_.backfill_complete(timestamp);
    backfilling_ = false;
}

void backfill_storer_t::realtime_get_cas(const store_key_t& key, castime_t castime) {
    block_pm_duration set_timer(&pm_slave_rt_get_cas);
    get_cas_mutation_t mut;
    mut.key = key;
    internal_store_.handover(new mutation_t(mut), castime);
}

void backfill_storer_t::realtime_sarc(const store_key_t& key, unique_ptr_t<data_provider_t> data,
        mcflags_t flags, exptime_t exptime, castime_t castime, add_policy_t add_policy,
        replace_policy_t replace_policy, cas_t old_cas) {
    block_pm_duration set_timer(&pm_slave_rt_sarc);

    sarc_mutation_t mut;
    mut.key = key;
    mut.data = data;
    mut.flags = flags;
    mut.exptime = exptime;
    mut.add_policy = add_policy;
    mut.replace_policy = replace_policy;
    mut.old_cas = old_cas;

    internal_store_.handover(new mutation_t(mut), castime);
}

void backfill_storer_t::realtime_incr_decr(incr_decr_kind_t kind, const store_key_t &key, uint64_t amount,
        castime_t castime) {
    block_pm_duration set_timer(&pm_slave_rt_incr_decr);
    incr_decr_mutation_t mut;
    mut.key = key;
    mut.kind = kind;
    mut.amount = amount;
    internal_store_.handover(new mutation_t(mut), castime);
}

void backfill_storer_t::realtime_append_prepend(append_prepend_kind_t kind, const store_key_t &key,
        unique_ptr_t<data_provider_t> data, castime_t castime) {
    block_pm_duration set_timer(&pm_slave_rt_app_prep);
    append_prepend_mutation_t mut;
    mut.key = key;
    mut.data = data;
    mut.kind = kind;
    internal_store_.handover(new mutation_t(mut), castime);
}

void backfill_storer_t::realtime_delete_key(const store_key_t &key, repli_timestamp timestamp) {
    block_pm_duration set_timer(&pm_slave_rt_del);
    delete_mutation_t mut;
    mut.key = key;
    mut.dont_put_in_delete_queue = true;
    // TODO: where does "timestamp" go???  IS THIS RIGHT?? WHO KNOWS.
    internal_store_.handover(new mutation_t(mut), castime_t(NO_CAS_SUPPLIED /* This isn't even used, why is it a parameter. */, timestamp));
}

void backfill_storer_t::realtime_time_barrier(repli_timestamp_t timestamp) {
    internal_store_.time_barrier(timestamp);
}

}
