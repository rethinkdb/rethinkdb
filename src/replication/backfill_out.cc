#include "btree/node.hpp"
#include "replication/backfill_out.hpp"
#include "concurrency/drain_semaphore.hpp"
#include "concurrency/pmap.hpp"
#include "concurrency/count_down_latch.hpp"
#include "btree/slice.hpp"
#include "concurrency/coro_pool.hpp"

perfmon_duration_sampler_t
    master_rt_get_cas("master_rt_get_cas", secs_to_ticks(1.0)),
    master_rt_sarc("master_rt_sarc", secs_to_ticks(1.0)),
    master_rt_incr_decr("master_rt_incr_decr", secs_to_ticks(1.0)),
    master_rt_app_prep("master_rt_app_prep", secs_to_ticks(1.0)),
    master_rt_del("master_rt_del", secs_to_ticks(1.0));

namespace replication {

struct backfill_and_streaming_manager_t :
    public backfill_callback_t,
    public home_thread_mixin_t
{
    /* Realtime operation handlers */

    struct change_visitor_t : public boost::static_visitor<mutation_t> {
    
        backfill_and_streaming_manager_t *manager;
        castime_t castime;
    
        mutation_t operator()(const get_cas_mutation_t& m) {
            manager->coro_pool.queue_task(boost::bind(&backfill_and_streaming_manager_t::realtime_get_cas, manager,
                    m.key, castime));
            return m;
        }
        mutation_t operator()(const sarc_mutation_t& m) {
            unique_ptr_t<data_provider_t> dps[2];
            duplicate_data_provider(m.data, 2, dps);
            manager->coro_pool.queue_task(boost::bind(&backfill_and_streaming_manager_t::realtime_sarc, manager,
                    m.key, dps[0], m.flags, m.exptime, castime, m.add_policy, m.replace_policy, m.old_cas));
            sarc_mutation_t m2(m);
            m2.data = dps[1];
            return m2;
        }
        mutation_t operator()(const incr_decr_mutation_t& m) {
            manager->coro_pool.queue_task(boost::bind(&backfill_and_streaming_manager_t::realtime_incr_decr, manager,
                    m.kind, m.key, m.amount, castime));
            return m;
        }
        mutation_t operator()(const append_prepend_mutation_t &m) {
            unique_ptr_t<data_provider_t> dps[2];
            duplicate_data_provider(m.data, 2, dps);
            manager->coro_pool.queue_task(boost::bind(&backfill_and_streaming_manager_t::realtime_append_prepend, manager,
                    m.kind, m.key, dps[0], castime));
            append_prepend_mutation_t m2(m);
            m2.data = dps[1];
            return m2;
        }
        mutation_t operator()(const delete_mutation_t& m) {
            manager->coro_pool.queue_task(boost::bind(&backfill_and_streaming_manager_t::realtime_delete_key, manager,
                    m.key, castime.timestamp));
            return m;
        }
    };

    mutation_t dispatch_change(const mutation_t& m, castime_t castime) {
        threadsafe_drain_semaphore_t::lock_t keep_alive(&realtime_mutation_drain_semaphore_);
        change_visitor_t functor;
        functor.manager = this;
        functor.castime = castime;
        return boost::apply_visitor(functor, m.mutation);
    }

    void realtime_get_cas(const store_key_t& key, castime_t castime) {
        block_pm_duration set_timer(&master_rt_get_cas);
        handler_->realtime_get_cas(key, castime);
    }
    void realtime_sarc(const store_key_t& key, unique_ptr_t<data_provider_t> data,
            mcflags_t flags, exptime_t exptime, castime_t castime, add_policy_t add_policy,
            replace_policy_t replace_policy, cas_t old_cas) {
        block_pm_duration set_timer(&master_rt_sarc);
        handler_->realtime_sarc(key, data, flags, exptime, castime, add_policy, replace_policy, old_cas);
    }
    void realtime_incr_decr(incr_decr_kind_t kind, const store_key_t &key, uint64_t amount,
            castime_t castime) {
        block_pm_duration set_timer(&master_rt_incr_decr);
        handler_->realtime_incr_decr(kind, key, amount, castime);
    }
    void realtime_append_prepend(append_prepend_kind_t kind, const store_key_t &key,
            unique_ptr_t<data_provider_t> data, castime_t castime) {
        block_pm_duration set_timer(&master_rt_app_prep);
        handler_->realtime_append_prepend(kind, key, data, castime);
    }
    void realtime_delete_key(const store_key_t &key, repli_timestamp timestamp) {
        block_pm_duration set_timer(&master_rt_del);
        handler_->realtime_delete_key(key, timestamp);
    }

    /* backfill_callback_t implementation */

    bool should_send_deletion_keys(bool can_send_deletion_keys) {
        on_thread_t th(home_thread);

        if (all_delete_queues_so_far_can_send_keys_ && !can_send_deletion_keys) {
            all_delete_queues_so_far_can_send_keys_ = false;

            // We only send this once, and it is important that we
            // send it before the delete_queues_can_send_keys_latch_
            // gets pulsed, because then a delete queue could finish
            // and start sending sets before we sent a
            // delete_everything message.
            handler_->backfill_delete_everything();
        }

        delete_queues_can_send_keys_latch_.count_down();

        // Wait until all slices have gotten here.
        delete_queues_can_send_keys_latch_.wait();

        return all_delete_queues_so_far_can_send_keys_;
    }

    void wait_and_maybe_send_delete_all_keys_message() {
        delete_queues_can_send_keys_latch_.wait();
    }

    /* The store calls this when we need to backfill a deletion. */
    void deletion_key(const btree_key_t *key) {
        // This runs on the scheduler thread.
        store_key_t tmp(key->size, key->contents);
        coro_pool.queue_task(boost::bind(&backfill_and_realtime_streaming_callback_t::backfill_deletion, handler_, tmp));
    }

    /* The store calls this when it finishes the first phase of backfilling. It's redundant
    because we will get another callback when the second phase is done. */
    void done_deletion_keys() {
    }

    /* The store calls this when we need to send a key/value pair to the slave */
    void on_keyvalue(backfill_atom_t atom) {
        // This runs on the scheduler thread.
        coro_pool.queue_task(boost::bind(&backfill_and_realtime_streaming_callback_t::backfill_set, handler_, atom));
    }

    /* When we are finally done, the store calls done(). */
    void done() {

        // This runs in the scheduler, so we have to spawn a coroutine.
        coro_pool.queue_task(boost::bind(&backfill_and_streaming_manager_t::do_done, this));
    }

    void do_done() {
        rassert(get_thread_id() == home_thread);

        outstanding_backfills--;
        if (outstanding_backfills == 0) {
            /* We're done backfilling. The timestamp we send to backfill_done() should be the
            earliest timestamp such that we have backfilled all changes which are timestamped
            less than that timestamp. That's why we call `.next()`; if it was "less than or equal"
            instead of "less than", we wouldn't call `.next()`. */
            debugf("Backfilled up to %d\n", initial_replication_clock_.next().time);
            handler_->backfill_done(initial_replication_clock_.next());

            /* This allows us to shut down */
            pulsed_when_backfill_over.pulse();
        }
    }

    /* Logic for incrementing the replication clock */

    repli_timestamp_t replication_clock_, initial_replication_clock_;

    void increment_replication_clock(drain_semaphore_t::lock_t) {

        /* We are sending heartbeat number "rc" */
        repli_timestamp_t rc = replication_clock_.next();

        debugf("Initiating transition from %d to %d\n", replication_clock_.time, rc.time);

        replication_clock_ = rc;

        /* This blocks while the heartbeat goes to the key-value stores and back. Before we made
        this call, every operation we got from the key-value stores had timestamp less than rc; when
        this call returns, every operation we get from the key-value stores will have timestamp
        greater than or equal to rc. */
        internal_store_->set_replication_clock(rc);

        /* Now that we're *sure* that no more operations are going to occur that have timestamps
        less than "rc", we can send a time-barrier to the slave. */
        debugf("Sending heartbeat %d to slave\n", rc.time);
        handler_->realtime_time_barrier(rc);
    }

    /* Startup, shutdown, and member variables */

    count_down_latch_t delete_queues_can_send_keys_latch_;
    bool all_delete_queues_so_far_can_send_keys_;

    btree_key_value_store_t *internal_store_;
    backfill_and_realtime_streaming_callback_t *handler_;

    threadsafe_drain_semaphore_t realtime_mutation_drain_semaphore_;

    int outstanding_backfills;

    boost::scoped_ptr<repeating_timer_t> replication_clock_timer_;
    drain_semaphore_t replication_clock_drain_semaphore_;

    /* In order to not stress the coroutine limit too much, we use a coro_pool to handle our work */
    coro_pool_t coro_pool;

    /* We can't stop until backfill is over and we can't interrupt a running backfill operation,
    so we have to use pulsed_when_backfill_over to wait for the backfill operation to finish. */
    cond_t pulsed_when_backfill_over;

    backfill_and_streaming_manager_t(btree_key_value_store_t *kvs,
            backfill_and_realtime_streaming_callback_t *handler,
            repli_timestamp_t timestamp) :
        delete_queues_can_send_keys_latch_(kvs->btree_static_config.n_slices),
        all_delete_queues_so_far_can_send_keys_(true),
        internal_store_(kvs),
        handler_(handler),
        coro_pool(512, 2048) // TODO: Make this a define-constant or something
    {
        /* Read the old value of the replication clock. */
        replication_clock_ = internal_store_->get_replication_clock();

        /* The time we start the backfill at */
        initial_replication_clock_ = replication_clock_;

        /* On each thread, atomically start a backfill operation and start streaming
        realtime operations. If we start a backfill but don't start streaming operations
        immediately, operations will be lost; if we start streaming operations before we
        start a backfill, operations will be duplicated. Both are bad. */

        outstanding_backfills = 0;

        for (int i = 0; i < internal_store_->btree_static_config.n_slices; i++) {

            outstanding_backfills++;

            // (we cannot use our coro_pool because it might run on a different thread)
            coro_t::spawn_on_thread(internal_store_->shards[i]->home_thread,
                boost::bind(&backfill_and_streaming_manager_t::register_on_slice,
                    this,
                    i,
                    timestamp
                    ));
        }

        /* Increment the replication clock right after we spawn the subcoroutines. This
        makes it so that every operation with timestamp `initial_replication_clock_` will be
        included in the backfill, and no operation with timestamp `initial_replication_clock_ + 1`
        will be included. */
        drain_semaphore_t::lock_t dont_shut_down(&replication_clock_drain_semaphore_);
        increment_replication_clock(dont_shut_down);

        /* Start the timer that will repeatedly increment the replication clock */
        replication_clock_timer_.reset(new repeating_timer_t(1000,
            boost::bind(&backfill_and_streaming_manager_t::increment_replication_clock, this, dont_shut_down)));
    }

    void register_on_slice(int i, repli_timestamp_t timestamp) {

        /* We must register for realtime updates atomically together with starting the
        backfill operation. */

        /* Register for real-time updates */
        internal_store_->shards[i]->dispatching_store.set_dispatcher(
            boost::bind(&backfill_and_streaming_manager_t::dispatch_change, this, _1, _2));

        /* Start a backfill operation */
        internal_store_->shards[i]->btree.backfill(timestamp, this);
    }

    ~backfill_and_streaming_manager_t() {

        debugf("~backfill_and_streaming_manager_t() 1\n");

        /* We can't delete ourself until the backfill is over. It's impossible to interrupt
        a running backfill. The backfill logic takes care of calling handler_->backfill_done(). */
        pulsed_when_backfill_over.wait();

        debugf("~backfill_and_streaming_manager_t() 2\n");

        /* Unregister for real-time updates */
        pmap(internal_store_->btree_static_config.n_slices,
            boost::bind(&backfill_and_streaming_manager_t::unregister_on_slice, this, _1));

        /* Stop the replication-clock timer, and wait for any operations that it started to
        finish. */
        replication_clock_timer_.reset();
        replication_clock_drain_semaphore_.drain();

        /* Wait for existing realtime updates to finish */
        realtime_mutation_drain_semaphore_.drain();

        debugf("~backfill_and_streaming_manager_t() 4\n");

        /* Now we can stop */
    }

    void unregister_on_slice(int i) {

        /* Unregister for realtime updates */
        internal_store_->shards[i]->dispatching_store.set_dispatcher(0);
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

}
