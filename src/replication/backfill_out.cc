#include "btree/node.hpp"
#include "replication/backfill_out.hpp"
#include "concurrency/drain_semaphore.hpp"
#include "concurrency/pmap.hpp"
#include "concurrency/count_down_latch.hpp"
#include "btree/slice.hpp"
#include "concurrency/coro_pool.hpp"
#include "concurrency/queue/cross_thread_limited_fifo.hpp"
#include "concurrency/queue/accounting.hpp"

perfmon_duration_sampler_t
    pm_replication_master_dispatch_cost("replication_master_dispatch_cost", secs_to_ticks(1.0)),
    pm_replication_master_enqueue_cost("replication_master_enqueue_cost", secs_to_ticks(1.0));

#define REPLICATION_JOB_QUEUE_DEPTH 2048

namespace replication {

struct backfill_and_streaming_manager_t :
    public home_thread_mixin_t
{
    /* We construct one `slice_manager_t` per slice */

    struct slice_manager_t :
        public backfill_callback_t
    {
        slice_manager_t(backfill_and_streaming_manager_t *parent, shard_store_t *shard, repli_timestamp_t backfill_from, repli_timestamp_t new_timestamp) :
            parent_(parent), shard_(shard),
            max_backfill_timestamp_(new_timestamp), min_realtime_timestamp_(new_timestamp),
            backfill_job_queue(shard_->home_thread, REPLICATION_JOB_QUEUE_DEPTH),
            realtime_job_queue(shard_->home_thread, REPLICATION_JOB_QUEUE_DEPTH),
            backfill_job_account(&parent->backfill_job_queue, &backfill_job_queue, 1),
            realtime_job_account(&parent->realtime_job_queue, &realtime_job_queue, 1)
        {
            on_thread_t thread_switcher(shard_->home_thread);

            {
                ASSERT_NO_CORO_WAITING;

                /* Increment the replication clock right as we start the backfill. This
                   makes it so that every operation with timestamp `new_timestamp - 1` will be
                   included in the backfill, and no operation with timestamp `new_timestamp`
                   will be included. */

                rassert(shard_->timestamper.get_timestamp() < new_timestamp, "shard_->timestamper.get_timestamp() = %u, new_timestamp = %u", shard_->timestamper.get_timestamp(), new_timestamp);

                shard_->timestamper.set_timestamp(new_timestamp);

                /* On each thread, atomically start a backfill operation and start streaming
                   realtime operations. If we start a backfill but don't start streaming operations
                   immediately, operations will be lost; if we start streaming operations before we
                   start a backfill, operations will be duplicated. Both are bad. */

                shard_->dispatching_store.set_dispatcher(
                    boost::bind(
                    &slice_manager_t::dispatch_change, this,
                    drain_semaphore_t::lock_t(&realtime_mutation_drain_semaphore),
                    _1, _2, _3));

                backfilling_ = true;
            }
            coro_t::spawn_now(boost::bind(
                &btree_slice_t::backfill, &shard->btree, backfill_from, this));
        }

        ~slice_manager_t() {
            on_thread_t thread_switcher(shard_->home_thread);

            shard_->dispatching_store.set_dispatcher(0);

            /* The backfill is already over, so there's no need to cancel the backfill.
            Besides, the backfill code doesn't support cancelling right now. */

            /* Wait for any already-dispatched changes to finish before we let ourselves
            be destroyed */
            realtime_mutation_drain_semaphore.drain();
        }

        backfill_and_streaming_manager_t *parent_;
        shard_store_t *shard_;

        // For sanity checking. All backfill operations should have timestamps less than
        // `max_backfill_timestamp_`, and all realtime operations should have timestamps
        // greater than or equal to `min_realtime_timestamp_`.
        repli_timestamp_t max_backfill_timestamp_, min_realtime_timestamp_;

        // True while actually backfilling
        bool backfilling_;

        cross_thread_limited_fifo_t<boost::function<void()> > backfill_job_queue, realtime_job_queue;
        accounting_queue_t<boost::function<void()> >::account_t backfill_job_account, realtime_job_account;

        drain_semaphore_t realtime_mutation_drain_semaphore;

        /* Functor for replicating changes */

        struct change_visitor_t : public boost::static_visitor<mutation_t> {

            slice_manager_t *manager;
            castime_t castime;
            order_token_t order_token;
            mutation_t operator()(const get_cas_mutation_t& m) {
                block_pm_duration timer(&pm_replication_master_enqueue_cost);
                manager->realtime_job_queue.push(boost::bind(
                    &backfill_and_realtime_streaming_callback_t::realtime_get_cas, manager->parent_->handler_,
                    m.key, castime, order_token));
                return m;
            }
            mutation_t operator()(const sarc_mutation_t& m) {
                unique_ptr_t<data_provider_t> dps[2];
                duplicate_data_provider(m.data, 2, dps);

                {
                    sarc_mutation_t m3(m);
                    m3.data = dps[0];
                    block_pm_duration timer(&pm_replication_master_enqueue_cost);
                    manager->realtime_job_queue.push(boost::bind(
                        &backfill_and_realtime_streaming_callback_t::realtime_sarc, manager->parent_->handler_,
                        m3, castime, order_token));
                }

                sarc_mutation_t m2(m);
                m2.data = dps[1];
                return m2;
            }
            mutation_t operator()(const incr_decr_mutation_t& m) {
                block_pm_duration timer(&pm_replication_master_enqueue_cost);
                manager->realtime_job_queue.push(boost::bind(
                    &backfill_and_realtime_streaming_callback_t::realtime_incr_decr, manager->parent_->handler_,
                    m.kind, m.key, m.amount, castime, order_token));
                return m;
            }
            mutation_t operator()(const append_prepend_mutation_t &m) {
                unique_ptr_t<data_provider_t> dps[2];
                duplicate_data_provider(m.data, 2, dps);
                block_pm_duration timer(&pm_replication_master_enqueue_cost);
                manager->realtime_job_queue.push(boost::bind(
                    &backfill_and_realtime_streaming_callback_t::realtime_append_prepend, manager->parent_->handler_,
                    m.kind, m.key, dps[0], castime, order_token));
                append_prepend_mutation_t m2(m);
                m2.data = dps[1];
                return m2;
            }
            mutation_t operator()(const delete_mutation_t& m) {
                block_pm_duration timer(&pm_replication_master_enqueue_cost);
                manager->realtime_job_queue.push(boost::bind(
                    &backfill_and_realtime_streaming_callback_t::realtime_delete_key, manager->parent_->handler_,
                    m.key, castime.timestamp, order_token));
                return m;
            }
        };

        mutation_t dispatch_change(
                UNUSED drain_semaphore_t::lock_t keep_alive,
                const mutation_t& m, castime_t castime, order_token_t token) {

            rassert(get_thread_id() == shard_->home_thread);
            rassert(castime.timestamp >= min_realtime_timestamp_);

            block_pm_duration timer(&pm_replication_master_dispatch_cost);
            change_visitor_t functor;
            functor.manager = this;
            functor.castime = castime;
            functor.order_token = token;
            return boost::apply_visitor(functor, m.mutation);
        }

        /* Logic for incrementing replication clock */

        void set_replication_clock(repli_timestamp_t new_timestamp, boost::function<void()> job) {
            on_thread_t thread_switcher(shard_->home_thread);

            rassert(new_timestamp >= min_realtime_timestamp_);
            min_realtime_timestamp_ = new_timestamp;

            shard_->timestamper.set_timestamp(new_timestamp);

            /* Push the notification through the realtime job queue so that the order of
            the notification is preserved relative to the realtime mutations that we
            push through the job queue. Every realtime mutation delivered before the
            notification is delivered will have a timestamp less than `new_timestamp`;
            every realtime mutation delivered after the notification is delivered will have
            timestamp greater than or equal to `new_timestamp`. */
            realtime_job_queue.push(job);
        }

        /* backfill_callback_t implementation */

        bool should_send_deletion_keys(bool can_send_deletion_keys) {
            on_thread_t th(parent_->home_thread);

            rassert(backfilling_);

            if (parent_->all_delete_queues_so_far_can_send_keys_ && !can_send_deletion_keys) {
                parent_->all_delete_queues_so_far_can_send_keys_ = false;

                // We only send this once, and it is important that we
                // send it before the delete_queues_can_send_keys_latch_
                // gets pulsed, because then a delete queue could finish
                // and start sending sets before we sent a
                // delete_everything message.
                parent_->handler_->backfill_delete_everything(order_token_t::ignore);
            }

            parent_->delete_queues_can_send_keys_latch_.count_down();

            // Wait until all slices have gotten here.
            parent_->delete_queues_can_send_keys_latch_.wait();

            return parent_->all_delete_queues_so_far_can_send_keys_;
        }

        void wait_and_maybe_send_delete_all_keys_message() {
            parent_->delete_queues_can_send_keys_latch_.wait();
        }

        /* The store calls this when we need to backfill a deletion. */
        void deletion_key(const btree_key_t *key) {
            // This runs in the scheduler context.
            rassert(get_thread_id() == shard_->home_thread);
            rassert(backfilling_);
            store_key_t tmp(key->size, key->contents);
            backfill_job_queue.push(boost::bind(
                &backfill_and_realtime_streaming_callback_t::backfill_deletion, parent_->handler_,
                tmp, order_token_t::ignore));
        }

        /* The store calls this when it finishes the first phase of backfilling. It's redundant
        because we will get another callback when the second phase is done. */
        void done_deletion_keys() {
        }

        /* The store calls this when we need to backfill a key/value pair to the slave */
        void on_keyvalue(backfill_atom_t atom) {
            // This runs in the scheduler context
            rassert(get_thread_id() == shard_->home_thread);
            rassert(atom.recency < max_backfill_timestamp_, "atom.recency (%u) < max_backfill_timestamp_ (%u)", atom.recency, max_backfill_timestamp_);
            rassert(backfilling_);
            backfill_job_queue.push(boost::bind(
                &backfill_and_realtime_streaming_callback_t::backfill_set, parent_->handler_,
                atom, order_token_t::ignore));
        }

        /* When we are finally done with the backfill, the store calls `done()`. */
        void done_backfill() {
            // This runs in the scheduler context. Since `push()` can block, we spawn a coroutine.
            // It is essential that we push it through the backfill job queue so that it it not
            // reordered relative to the actual backfill sets. Note that `boost::bind()` assigns
            // a special meaning to `boost::bind()` called with a `boost::bind()` as one of its
            // parameters, so we need to wrap it in a `boost::function` to get the behavior we
            // want.
            rassert(get_thread_id() == shard_->home_thread);
            rassert(backfilling_);
            backfilling_ = false;
            boost::function<void()> fun = boost::bind(&backfill_and_streaming_manager_t::backfill_done, parent_);
            coro_t::spawn_now(boost::bind(
                &cross_thread_limited_fifo_t<boost::function<void()> >::push, &backfill_job_queue, fun));
        }
    };

    void backfill_done() {
        rassert(get_thread_id() == home_thread);

        outstanding_backfills--;
        if (outstanding_backfills == 0) {

            /* We're done backfilling. The timestamp we send to backfill_done() should be the
            earliest timestamp such that we have backfilled all changes which are timestamped
            less than that timestamp. That's why we call `.next()`; if it was "less than or equal"
            instead of "less than", we wouldn't call `.next()`. */
            debugf("Backfilled up to %d\n", initial_replication_clock_.next().time);
            handler_->backfill_done(initial_replication_clock_.next(), order_token_t::ignore);

            /* This allows us to shut down */
            pulsed_when_backfill_over.pulse();
        }
    }

    /* Logic for incrementing the replication clock */

    repli_timestamp_t replication_clock_;
    repli_timestamp_t initial_replication_clock_;   // The time we started the backfill at

    void increment_replication_clock(drain_semaphore_t::lock_t) {

        /* We are sending heartbeat number "rc" */
        repli_timestamp_t rc = replication_clock_.next();

        /* Record the new value of the replication clock */
        replication_clock_ = rc;
        internal_store_->set_replication_clock(rc);

        /* `slice_manager_t::set_replication_clock()` pushes a command to count down the
        `count_down_latch_t` through the same queue that is used for realtime replication
        operations. That guarantees correct ordering between replication operations and
        replication clock ticks. */
        count_down_latch_t countdown(slice_managers.size());
        for (int i = 0; i < (int)slice_managers.size(); i++) {
            slice_managers[i]->set_replication_clock(rc,
                boost::bind(&count_down_latch_t::count_down, &countdown));
        }
        countdown.wait();

        /* Now we're *sure* that no more operations are going to occur that have timestamps
        less than "rc", so we can send a time-barrier to the slave. */

        handler_->realtime_time_barrier(rc, order_token_t::ignore);
    }

    /* Startup, shutdown, and member variables */

    count_down_latch_t delete_queues_can_send_keys_latch_;
    bool all_delete_queues_so_far_can_send_keys_;

    btree_key_value_store_t *internal_store_;
    backfill_and_realtime_streaming_callback_t *handler_;

    int outstanding_backfills;

    std::vector<slice_manager_t *> slice_managers;

    boost::scoped_ptr<repeating_timer_t> replication_clock_timer_;
    drain_semaphore_t replication_clock_drain_semaphore_;

    /* In order to not stress the coroutine limit too much, we use a `coro_pool_t` to handle
    our work. We feed the coro_pool_t from an `accounting_queue_t` with two accounts, one for
    realtime operations and one for backfills. This way, backfills don't choke out realtime
    operations or vice versa. Each of the accounts is itself an `accounting_queue_t` that
    collects operations from all the different slices. */
    accounting_queue_t<boost::function<void()> > combined_job_queue;
    coro_pool_t coro_pool;
    accounting_queue_t<boost::function<void()> > backfill_job_queue, realtime_job_queue;
    accounting_queue_t<boost::function<void()> >::account_t backfill_job_account, realtime_job_account;

    /* We can't stop until backfill is over and we can't interrupt a running backfill operation,
    so we have to use pulsed_when_backfill_over to wait for the backfill operation to finish. */
    cond_t pulsed_when_backfill_over;

    backfill_and_streaming_manager_t(btree_key_value_store_t *kvs,
            backfill_and_realtime_streaming_callback_t *handler,
            repli_timestamp_t backfill_from) :
        delete_queues_can_send_keys_latch_(kvs->btree_static_config.n_slices),
        all_delete_queues_so_far_can_send_keys_(true),
        internal_store_(kvs),
        handler_(handler),
        /* Every job that goes through the job queue will try to acquire the same lock, so
        there is no point in having more than one coroutine. Also, there is empirical
        evidence that having extra coroutines just waiting for the lock hurts performance.
        So the "pool" really just has one thing in it. */
        coro_pool(1, &combined_job_queue),
        backfill_job_account(&combined_job_queue, &backfill_job_queue, 1),
        realtime_job_account(&combined_job_queue, &realtime_job_queue, 1)
    {
        /* Read the old value of the replication clock. */
        initial_replication_clock_ = internal_store_->get_replication_clock();

        slice_managers.resize(internal_store_->btree_static_config.n_slices);
        replication_clock_ = initial_replication_clock_.next();
        outstanding_backfills = internal_store_->btree_static_config.n_slices;

        pmap(internal_store_->btree_static_config.n_slices,
             boost::bind(&backfill_and_streaming_manager_t::register_on_slice, this,
                         _1, backfill_from, replication_clock_));

        /* Store the current state of the replication clock */
        internal_store_->set_replication_clock(replication_clock_);

        /* Start the timer that will repeatedly increment the replication clock */
        replication_clock_timer_.reset(new repeating_timer_t(1000,
            boost::bind(
                &backfill_and_streaming_manager_t::increment_replication_clock, this,
                drain_semaphore_t::lock_t(&replication_clock_drain_semaphore_))));
    }

    void register_on_slice(int i, repli_timestamp_t backfill_from, repli_timestamp_t new_timestamp) {
        slice_managers[i] = new slice_manager_t(this, internal_store_->shards[i], backfill_from, new_timestamp);
    }

    ~backfill_and_streaming_manager_t() {

        /* We can't delete ourself until the backfill is over. It's impossible to interrupt
        a running backfill. The backfill logic takes care of calling handler_->backfill_done(). */
        pulsed_when_backfill_over.wait();

        /* Stop the replication-clock timer, and wait for any operations that it started to
        finish. We must do this before we delete the slice managers. */
        replication_clock_timer_.reset();
        replication_clock_drain_semaphore_.drain();

        /* Unregister for real-time updates */
        pmap(internal_store_->btree_static_config.n_slices,
            boost::bind(&backfill_and_streaming_manager_t::unregister_on_slice, this, _1));

        /* Now we can stop */
    }

    void unregister_on_slice(int i) {
        delete slice_managers[i];
    }
};

void backfill_and_realtime_stream(btree_key_value_store_t *kvs, repli_timestamp_t start_time,
        backfill_and_realtime_streaming_callback_t *bfh, signal_t *pulse_to_stop) {

    {
        /* Constructing this object starts the backfill and the streaming. */
        backfill_and_streaming_manager_t manager(kvs, bfh, start_time);

        pulse_to_stop->wait();

        /* The destructor for "manager" waits for the backfill to finish, then stops the
        realtime stream. */
    }
}

}  // namespace replication
