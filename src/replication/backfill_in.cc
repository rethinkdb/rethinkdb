#include "replication/backfill_in.hpp"

namespace replication {

perfmon_counter_t pm_replication_slave_realtime_queue("replication_slave_realtime_queue");

#define BACKFILL_QUEUE_CAPACITY 2048
#define REALTIME_QUEUE_CAPACITY 2048
#define CORO_POOL_SIZE 512

backfill_storer_t::backfill_storer_t(btree_key_value_store_t *underlying) :
    kvs_(underlying), backfilling_(true), print_backfill_warning_(false),
    backfill_queue_(BACKFILL_QUEUE_CAPACITY),
    realtime_queue_(SEMAPHORE_NO_LIMIT, 0.5, &pm_replication_slave_realtime_queue),
    queue_picker_(make_vector<passive_producer_t<boost::function<void()> > *>(&backfill_queue_)),
    coro_pool_(CORO_POOL_SIZE, &queue_picker_)
    { }

backfill_storer_t::~backfill_storer_t() {
    if (print_backfill_warning_) {
        logWRN("A backfill operation is being interrupted. The data in this database is now "
            "in an inconsistent state. To get the data back into a consistent state, "
            "reestablish the master-slave connection and allow the backfill to run to "
            "completion.\n");
    }
}

void backfill_storer_t::backfill_delete_everything(order_token_t token) {
    order_sink_.check_out(token);
    print_backfill_warning_ = true;
    backfill_queue_.push(boost::bind(
        &btree_key_value_store_t::delete_all_keys_for_backfill, kvs_));
}

void backfill_storer_t::backfill_deletion(store_key_t key, order_token_t token) {
    order_sink_.check_out(token);
    print_backfill_warning_ = true;

    delete_mutation_t mut;
    mut.key = key;
    mut.dont_put_in_delete_queue = true;
    // NO_CAS_SUPPLIED is not used in any way for deletions, and the
    // timestamp is part of the "->change" interface in a way not
    // relevant to slaves -- it's used when putting deletions into the
    // delete queue.
    backfill_queue_.push(boost::bind(
        &btree_key_value_store_t::change, kvs_,
        mut, castime_t(NO_CAS_SUPPLIED, repli_timestamp::invalid), order_token_t::ignore
        ));
}

void backfill_storer_t::backfill_set(backfill_atom_t atom, order_token_t token) {
    order_sink_.check_out(token);
    print_backfill_warning_ = true;

    sarc_mutation_t mut;
    mut.key = atom.key;
    mut.data = atom.value;
    mut.flags = atom.flags;
    mut.exptime = atom.exptime;
    mut.add_policy = add_policy_yes;
    mut.replace_policy = replace_policy_yes;
    mut.old_cas = NO_CAS_SUPPLIED;
    backfill_queue_.push(boost::bind(
        &btree_key_value_store_t::change, kvs_,
        mut, castime_t(atom.cas_or_zero, atom.recency), order_token_t::ignore
        ));

    if (atom.cas_or_zero != 0) {
        /* We need to make sure that the key gets assigned a CAS, because it has a CAS on the
        other node. If the key did not have a CAS before we sent "mut", then it will still lack a
        CAS after we send "mut", so we also send "cas_mut" to force it to have a CAS. Since we
        send "atom.cas_or_zero" as the "proposed_cas" for "cas_mut", the CAS will be correct. If
        the key had a CAS before we sent "mut", then "mut" will set the CAS to the correct value,
        and "cas_mut" will be a noop. */
        get_cas_mutation_t cas_mut;
        cas_mut.key = mut.key;
        backfill_queue_.push(boost::bind(
            &btree_key_value_store_t::change, kvs_,
            cas_mut, castime_t(atom.cas_or_zero, atom.recency), order_token_t::ignore));
    }
}

void backfill_storer_t::backfill_done(repli_timestamp_t timestamp, order_token_t token) {
    order_sink_.check_out(token);
    rassert(backfilling_);
    backfilling_ = false;
    print_backfill_warning_ = false;

    /* Write replication clock before timestamp so that if the flush happens
    between them, we will redo the backfill instead of proceeding with a wrong
    replication clock. */
    backfill_queue_.push(boost::bind(
        &btree_key_value_store_t::set_replication_clock, kvs_, timestamp));
    backfill_queue_.push(boost::bind(
        &btree_key_value_store_t::set_last_sync, kvs_, timestamp));

    /* We want the realtime queue to accept operations without throttling until
    the backfill is over, and then to start throttling. To make sure that we
    start throttling right when the realtime queue actually starts draining, we
    make the final operation on the backfilling queue be the operation that
    enables throttling on the realtime queue. */
    backfill_queue_.push(boost::bind(
        &limited_fifo_queue_t<boost::function<void()> >::set_capacity, &realtime_queue_,
        REALTIME_QUEUE_CAPACITY));

    /* Allow the `listing_passive_producer_t` to run operations from the
    `realtime_queue_` once the `backfill_queue_` is empty. */
    queue_picker_.set_sources(
        make_vector<passive_producer_t<boost::function<void()> > *>(
            &backfill_queue_, &realtime_queue_));
}

void backfill_storer_t::realtime_get_cas(const store_key_t& key, castime_t castime, order_token_t token) {
    order_sink_.check_out(token);
    get_cas_mutation_t mut;
    mut.key = key;
    realtime_queue_.push(boost::bind(&btree_key_value_store_t::change, kvs_, mut, castime, token));
}

void backfill_storer_t::realtime_sarc(sarc_mutation_t& m, castime_t castime, order_token_t token) {
    order_sink_.check_out(token);
    realtime_queue_.push(boost::bind(&btree_key_value_store_t::change, kvs_, m, castime, token));
}

void backfill_storer_t::realtime_incr_decr(incr_decr_kind_t kind, const store_key_t &key, uint64_t amount,
                                           castime_t castime, order_token_t token) {
    order_sink_.check_out(token);
    incr_decr_mutation_t mut;
    mut.key = key;
    mut.kind = kind;
    mut.amount = amount;
    realtime_queue_.push(boost::bind(&btree_key_value_store_t::change, kvs_, mut, castime, token));
}

void backfill_storer_t::realtime_append_prepend(append_prepend_kind_t kind, const store_key_t &key,
                                                unique_ptr_t<data_provider_t> data, castime_t castime, order_token_t token) {
    order_sink_.check_out(token);
    append_prepend_mutation_t mut;
    mut.key = key;
    mut.data = data;
    mut.kind = kind;
    realtime_queue_.push(boost::bind(&btree_key_value_store_t::change, kvs_, mut, castime, token));
}

void backfill_storer_t::realtime_delete_key(const store_key_t &key, repli_timestamp timestamp, order_token_t token) {
    order_sink_.check_out(token);
    delete_mutation_t mut;
    mut.key = key;
    mut.dont_put_in_delete_queue = true;
    // TODO: where does "timestamp" go???  IS THIS RIGHT?? WHO KNOWS.
    realtime_queue_.push(boost::bind(
        &btree_key_value_store_t::change, kvs_, mut,
        castime_t(NO_CAS_SUPPLIED /* This isn't even used, why is it a parameter. */, timestamp),
        token
        ));
}

void backfill_storer_t::realtime_time_barrier(repli_timestamp_t timestamp, UNUSED order_token_t token) {
    // TODO order_token_t: use the token.
    /* Write replication clock before timestamp so that if the flush happens
    between them, we will redo the backfill instead of proceeding with a wrong
    replication clock. */
    realtime_queue_.push(boost::bind(&btree_key_value_store_t::set_replication_clock, kvs_, timestamp));
    realtime_queue_.push(boost::bind(&btree_key_value_store_t::set_last_sync, kvs_, timestamp));
}

}
