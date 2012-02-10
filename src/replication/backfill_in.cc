#include "replication/backfill_in.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "logger.hpp"

#ifndef NDEBUG
// We really shouldn't include this from here (the dependencies are
// backwards), but we need it for
// master_t::inside_backfill_done_or_backfill.
#include "replication/master.hpp"
#endif

template<class value_t>
std::vector<value_t> make_vector(value_t v1) {
    std::vector<value_t> vec;
    vec.push_back(v1);
    return vec;
}

template<class value_t>
std::vector<value_t> make_vector(value_t v1, value_t v2) {
    std::vector<value_t> vec;
    vec.push_back(v1);
    vec.push_back(v2);
    return vec;
}

namespace replication {

// Stats for the current depth of realtime and backfill queues.
// The `replication_slave_realtime_queue` stat is mentioned in the manual, so if
// you change it, be sure to update the manual as well.
perfmon_counter_t
    pm_replication_slave_realtime_queue("replication_slave_realtime_queue", false),
    pm_replication_slave_backfill_queue("replication_slave_backfill_queue");

// Stats for how long it takes to push something onto the queue (it should be
// very fast unless the queue is backed up)
perfmon_duration_sampler_t
    pm_replication_slave_realtime_enqueue("replication_slave_realtime_enqueue", secs_to_ticks(1.0)),
    pm_replication_slave_backfill_enqueue("replication_slave_backfill_enqueue", secs_to_ticks(1.0));

#define BACKFILL_QUEUE_CAPACITY 2048
#define REALTIME_QUEUE_CAPACITY 2048
#define CORO_POOL_SIZE 512

backfill_storer_t::backfill_storer_t(sequence_group_t *replication_seq_group, btree_key_value_store_t *underlying) :
    seq_group_(replication_seq_group), kvs_(underlying), backfilling_(false), print_backfill_warning_(false),
    backfill_queue_(BACKFILL_QUEUE_CAPACITY),
    realtime_queue_(SEMAPHORE_NO_LIMIT, 0.5, &pm_replication_slave_realtime_queue),
    queue_picker_(&backfill_queue_),
    coro_pool_(CORO_POOL_SIZE, &queue_picker_)
{
}

backfill_storer_t::~backfill_storer_t() {
    if (print_backfill_warning_) {
        logWRN("A backfill operation is being interrupted. The data in this database is now "
            "in an inconsistent state. To get the data back into a consistent state, "
            "reestablish the master-slave connection and allow the backfill to run to "
            "completion.\n");
    }
}

void backfill_storer_t::ensure_backfilling() {
    if (!backfilling_) {
        // Make sure that realtime operations are not processed until we finish
        // backfilling

        // In order to do that, we first drain all realtime operations that are
        // still lingering around (is it possible at all that there are any?).
        // Then we set the queue_picker_ to only take requests from the backfill queue.
        debugf("backfill_storer_t:      Draining coro_pool_ for realtime operations to finish.\n");
        coro_pool_.drain();
        debugf("backfill_storer_t: DONE Draining coro_pool_ for realtime operations to finish.\n");
        queue_picker_.set_source(&backfill_queue_);
    }
    backfilling_ = true;
}

void backfill_storer_t::backfill_delete_range(int hash_value, int hashmod, bool left_key_supplied, const store_key_t& left_key_exclusive, bool right_key_supplied, const store_key_t& right_key_inclusive, order_token_t token) {
    print_backfill_warning_ = true;
    ensure_backfilling();

    block_pm_duration timer(&pm_replication_slave_backfill_enqueue);
    backfill_queue_.push(boost::bind(&btree_key_value_store_t::backfill_delete_range, kvs_, seq_group_, hash_value, hashmod, left_key_supplied, left_key_exclusive, right_key_supplied, right_key_inclusive, token));
}

void backfill_storer_t::backfill_deletion(store_key_t key, repli_timestamp_t timestamp, order_token_t token) {
    print_backfill_warning_ = true;
    ensure_backfilling();

    delete_mutation_t mut;
    mut.key = key;
    mut.dont_put_in_delete_queue = true;
    block_pm_duration timer(&pm_replication_slave_backfill_enqueue);

    backfill_queue_.push(boost::bind(
        &btree_key_value_store_t::change, kvs_,
        seq_group_,
        mutation_t(mut),
        // NO_CAS_SUPPLIED is not used in any way for deletions, and the
        // timestamp is part of the "->change" interface in a way not
        // relevant to slaves -- it's used when putting deletions into the
        // delete queue.
        castime_t(NO_CAS_SUPPLIED, timestamp), token
        ));
}

void backfill_storer_t::backfill_set(backfill_atom_t atom, order_token_t token) {
    print_backfill_warning_ = true;
    ensure_backfilling();

    sarc_mutation_t mut;
    mut.key = atom.key;
    mut.data = atom.value;
    mut.flags = atom.flags;
    mut.exptime = atom.exptime;
    mut.add_policy = add_policy_yes;
    mut.replace_policy = replace_policy_yes;
    mut.old_cas = NO_CAS_SUPPLIED;
    block_pm_duration timer(&pm_replication_slave_backfill_enqueue);

    backfill_queue_.push(boost::bind(
        &btree_key_value_store_t::change, kvs_, seq_group_,
        mut, castime_t(atom.cas_or_zero, atom.recency), token
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
        block_pm_duration timer(&pm_replication_slave_backfill_enqueue);
        backfill_queue_.push(boost::bind(
            &btree_key_value_store_t::change, kvs_, seq_group_,
            cas_mut, castime_t(atom.cas_or_zero, atom.recency), token));
    }
}

void backfill_storer_t::backfill_done(repli_timestamp_t timestamp, order_token_t token) {
#ifndef NDEBUG
    rassert(!master_t::inside_backfill_done_or_backfill);
    master_t::inside_backfill_done_or_backfill = true;
#endif

    /* Call ensure_backfilling() a last time just to make sure that the queue_picker_ is set
     up correctly (i.e. does not process the realtime queue yet). Otherwise draining the coro_pool
     as we do below wouldn't work as expected. */
    ensure_backfilling();
    print_backfill_warning_ = false;

    backfill_queue_.push(boost::bind(
        &btree_key_value_store_t::set_timestampers, kvs_, timestamp));

    /* Write replication clock before timestamp so that if the flush happens
    between them, we will redo the backfill instead of proceeding with a wrong
    replication clock. */
    backfill_queue_.push(boost::bind(&btree_key_value_store_t::set_replication_clock, kvs_, seq_group_, timestamp, token));
    backfill_queue_.push(boost::bind(&btree_key_value_store_t::set_last_sync, kvs_, seq_group_, timestamp, token));

    /* We want the realtime queue to accept operations without throttling until
    the backfill is over, and then to start throttling. To make sure that we
    start throttling right when the realtime queue actually starts draining, we
    make the final operation on the backfilling queue be the operation that
    enables throttling on the realtime queue. */
    backfill_queue_.push(boost::bind(
        &limited_fifo_queue_t<boost::function<void()> >::set_capacity, &realtime_queue_,
        REALTIME_QUEUE_CAPACITY));

    // We need to make sure all the backfill queue operations are finished
    // before we return, before the net_backfill_t handler gets
    // called.
    // As we are sure that queue_picker_ does only fetch requests from the backfill
    // queue at the moment (see call to ensure_backfilling() above), we can do that
    // by draining the coro pool which processes the requests.
    // Note: This does not allow any new requests to get in on the backfill queue
    // while draining, but that should be ok.
    // TODO: The cleaner way of implementing this semantics would be to pass
    // a lock object with each operation we push on the backfill_queue. That lock's
    // lifespan would be until the corresponding operation finishes. We would then
    // wait until all the locks have been released.
    debugf("backfill_storer_t:      Draining coro_pool_ to make sure that backfill operations finish.\n");
    coro_pool_.drain();
    debugf("backfill_storer_t: DONE Draining coro_pool_ to make sure that backfill operations finish.\n");
    backfilling_ = false;

    /* Allow the `listing_passive_producer_t` to run operations from the
    `realtime_queue_` once the `backfill_queue_` is empty (which it actually should
     be anyway by now). */
    queue_picker_.set_source(&realtime_queue_);

#ifndef NDEBUG
    master_t::inside_backfill_done_or_backfill = false;
#endif
}

void backfill_storer_t::realtime_get_cas(const store_key_t& key, castime_t castime, order_token_t token) {
    get_cas_mutation_t mut;
    mut.key = key;
    block_pm_duration timer(&pm_replication_slave_realtime_enqueue);

    realtime_queue_.push(boost::bind(&btree_key_value_store_t::change, kvs_, seq_group_, mut, castime, token));
}

void backfill_storer_t::realtime_sarc(sarc_mutation_t& m, castime_t castime, order_token_t token) {
    block_pm_duration timer(&pm_replication_slave_realtime_enqueue);

    realtime_queue_.push(boost::bind(&btree_key_value_store_t::change, kvs_, seq_group_, m, castime, token));
}

void backfill_storer_t::realtime_incr_decr(incr_decr_kind_t kind, const store_key_t &key, uint64_t amount,
                                           castime_t castime, order_token_t token) {
    incr_decr_mutation_t mut;
    mut.key = key;
    mut.kind = kind;
    mut.amount = amount;
    block_pm_duration timer(&pm_replication_slave_realtime_enqueue);

    realtime_queue_.push(boost::bind(&btree_key_value_store_t::change, kvs_, seq_group_, mut, castime, token));
}

void backfill_storer_t::realtime_append_prepend(append_prepend_kind_t kind, const store_key_t &key,
                                                const boost::intrusive_ptr<data_buffer_t>& data, castime_t castime, order_token_t token) {
    append_prepend_mutation_t mut;
    mut.key = key;
    mut.data = data;
    mut.kind = kind;
    block_pm_duration timer(&pm_replication_slave_realtime_enqueue);

    realtime_queue_.push(boost::bind(&btree_key_value_store_t::change, kvs_, seq_group_, mut, castime, token));
}

void backfill_storer_t::realtime_delete_key(const store_key_t &key, repli_timestamp_t timestamp, order_token_t token) {
    delete_mutation_t mut;
    mut.key = key;
    mut.dont_put_in_delete_queue = true;
    block_pm_duration timer(&pm_replication_slave_realtime_enqueue);

    realtime_queue_.push(boost::bind(
        &btree_key_value_store_t::change, kvs_, seq_group_, mut,
        // TODO: where does "timestamp" go???  IS THIS RIGHT?? WHO KNOWS.
        castime_t(NO_CAS_SUPPLIED /* This isn't even used, why is it a parameter. */, timestamp),
        token
        ));
}

void backfill_storer_t::realtime_time_barrier(repli_timestamp_t timestamp, order_token_t token) {
    /* Write the replication clock before writing `last_sync` so that if we crash between
    them, we'll re-sync that second instead of proceeding with a wrong replication clock.
    There is no need to change the timestamper because there are no sets being sent to this
    node; the master is still up. */
    block_pm_duration timer(&pm_replication_slave_realtime_enqueue);
    realtime_queue_.push(boost::bind(&btree_key_value_store_t::set_replication_clock, kvs_, seq_group_, timestamp, token));
    realtime_queue_.push(boost::bind(&btree_key_value_store_t::set_last_sync, kvs_, seq_group_, timestamp, token));
}

}
