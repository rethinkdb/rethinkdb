#include "btree/node.hpp"
#include "replication/backfill.hpp"
#include "concurrency/drain_semaphore.hpp"
#include "concurrency/pmap.hpp"

namespace replication {

struct backfill_and_streaming_manager_t :
    public backfill_callback_t,
    public home_thread_mixin_t
{
    /* Realtime operation handlers */

    /* realtime_mutation_dispatcher_t is used to listen for realtime mutations on the key-value
    store. */
    
    struct realtime_mutation_dispatcher_t : public mutation_dispatcher_t {
    
        struct change_visitor_t : public boost::static_visitor<mutation_t> {
        
            backfill_and_streaming_manager_t *manager;
            castime_t castime;
        
            mutation_t operator()(const get_cas_mutation_t& m) {
                coro_t::spawn_on_thread(manager->home_thread,
                    boost::bind(&backfill_and_streaming_manager_t::realtime_get_cas, manager,
                        m.key, castime));
                return m;
            }
            mutation_t operator()(const sarc_mutation_t& m) {
                unique_ptr_t<data_provider_t> dps[2];
                duplicate_data_provider(m.data, 2, dps);
                coro_t::spawn_on_thread(manager->home_thread,
                    boost::bind(&backfill_and_streaming_manager_t::realtime_sarc, manager,
                        m.key, dps[0], m.flags, m.exptime, castime, m.add_policy, m.replace_policy, m.old_cas));
                sarc_mutation_t m2(m);
                m2.data = dps[1];
                return m2;
            }
            mutation_t operator()(const incr_decr_mutation_t& m) {
                coro_t::spawn_on_thread(manager->home_thread,
                    boost::bind(&backfill_and_streaming_manager_t::realtime_incr_decr, manager,
                        m.kind, m.key, m.amount, castime));
                return m;
            }
            mutation_t operator()(const append_prepend_mutation_t &m) {
                unique_ptr_t<data_provider_t> dps[2];
                duplicate_data_provider(m.data, 2, dps);
                coro_t::spawn_on_thread(manager->home_thread,
                    boost::bind(&backfill_and_streaming_manager_t::realtime_append_prepend, manager,
                        m.kind, m.key, dps[0], castime));
                append_prepend_mutation_t m2(m);
                m2.data = dps[1];
                return m2;
            }
            mutation_t operator()(const delete_mutation_t& m) {
                coro_t::spawn_on_thread(manager->home_thread,
                    boost::bind(&backfill_and_streaming_manager_t::realtime_delete_key, manager,
                        m.key, castime.timestamp));
                return m;
            }
        };
    
        mutation_t dispatch_change(const mutation_t& m, castime_t castime) {
            change_visitor_t functor;
            functor.manager = manager_;
            functor.castime = castime;
            return boost::apply_visitor(functor, m.mutation);
        }
    
        backfill_and_streaming_manager_t *manager_;
    };

    void realtime_get_cas(const store_key_t& key, castime_t castime) {
        note_timestamp(castime.timestamp);
        handler_->realtime_get_cas(key, castime);
    }
    void realtime_sarc(const store_key_t& key, unique_ptr_t<data_provider_t> data,
            mcflags_t flags, exptime_t exptime, castime_t castime, add_policy_t add_policy,
            replace_policy_t replace_policy, cas_t old_cas) {
        note_timestamp(castime.timestamp);
        handler_->realtime_sarc(key, data, flags, exptime, castime, add_policy, replace_policy, old_cas);
    }
    void realtime_incr_decr(incr_decr_kind_t kind, const store_key_t &key, uint64_t amount,
            castime_t castime) {
        note_timestamp(castime.timestamp);
        handler_->realtime_incr_decr(kind, key, amount, castime);
    }
    void realtime_append_prepend(append_prepend_kind_t kind, const store_key_t &key,
            unique_ptr_t<data_provider_t> data, castime_t castime) {
        note_timestamp(castime.timestamp);
        handler_->realtime_append_prepend(kind, key, data, castime);
    }
    void realtime_delete_key(const store_key_t &key, repli_timestamp timestamp) {
        note_timestamp(timestamp);
        handler_->realtime_delete_key(key, timestamp);
    }

    /* As we receive real-time operations, we need to determine when we have received the
    last operation whose timestamp is in a given second. The way we do this is: whenever we
    see a timestamp T that we have never seen before, we do a round-trip to every thread to
    make sure that all of the messages with timestamp T-1 have made it to the
    backfill_and_streaming_manager_t, and then we send a message to the listener saying that second
    T-1 is over.

    There are two things that drive this: we check whenever we receive a mutation, and if we don't
    receive mutations for a while we just check periodically. */

    static void idle_timer_callback(void *sender_) {
        backfill_and_streaming_manager_t *sender = reinterpret_cast<backfill_and_streaming_manager_t *>(sender_);

        sender->idle_timer_ = NULL;
        sender->note_timestamp(current_time());
    }

    void note_timestamp(repli_timestamp timestamp) {

        assert_thread();
        rassert(timestamp.time != repli_timestamp::invalid.time);

        if (timestamp.time > latest_timestamp_.time) {
            latest_timestamp_ = timestamp;
            coro_t::spawn_now(boost::bind(&backfill_and_streaming_manager_t::do_time_barrier, this, timestamp));
        }

        /* We have a timer in addition to the notifications as we receive operations so that even
        if there are no operations for a while, we still send heartbeats to the slave. */
        if (idle_timer_) cancel_timer(idle_timer_);
        idle_timer_ = fire_timer_once(1100, &backfill_and_streaming_manager_t::idle_timer_callback, this);
    }

    static void roundtrip_to_db_thread(int slice_thread, repli_timestamp timestamp, cond_t *cond, int *counter) {
        {
            on_thread_t th(slice_thread);

            repli_timestamp t = current_time();

            // TODO: Don't crash just because the slave sent a bunch of
            // crap to us.  Just disconnect the slave.
            guarantee(t.time >= timestamp.time);
        }

        --*counter;
        rassert(*counter >= 0);
        if (*counter == 0) {
            cond->pulse();
        }
    }

    void do_time_barrier(repli_timestamp t) {

        assert_thread();

        // So we don't get deleted while we wait for the thread round-trips
        time_barriers_drain_semaphore_.acquire();

        // TODO: Use pmap
        cond_t cond;
        int n = internal_store_->btree_static_config.n_slices;
        int counter = n;
        for (int i = 0; i < n; i++) {
            coro_t::spawn(boost::bind(&backfill_and_streaming_manager_t::roundtrip_to_db_thread,
                internal_store_->btrees[i]->home_thread, t, &cond, &counter));
        }
        cond.wait();

        handler_->realtime_time_barrier(t);

        time_barriers_drain_semaphore_.release();
    }

    /* backfill_callback_t implementation */

    /* The store calls this when we need to backfill a deletion. */
    void deletion_key(const btree_key_t *key) {
        // This runs on the scheduler thread.
        store_key_t tmp(key->size, key->contents);
        coro_t::spawn_on_thread(home_thread,
            boost::bind(&backfill_and_realtime_streaming_callback_t::backfill_deletion, handler_, tmp));
    }

    /* The store calls this when it finishes the first phase of backfilling. It's redundant
    because we will get another callback when the second phase is done. */
    void done_deletion_keys() {
    }

    /* The store calls this when we need to send a key/value pair to the slave */
    void on_keyvalue(backfill_atom_t atom) {
        // This runs on the scheduler thread.
        coro_t::spawn_on_thread(home_thread,
            boost::bind(&backfill_and_realtime_streaming_callback_t::backfill_set, handler_, atom));
    }

    /* When we are finally done, the store calls done() with the timestamp that the operation
    started at. */
    void done(repli_timestamp oper_start_timestamp) {
        // This runs on the scheduler thread.
        rassert(oper_start_timestamp != repli_timestamp_t::invalid);
        coro_t::spawn_on_thread(home_thread,
            boost::bind(&backfill_and_streaming_manager_t::do_done, this, oper_start_timestamp));
    }

    void do_done(repli_timestamp_t oper_start_timestamp) {
        rassert(get_thread_id() == home_thread);

        /* We want to send the most conservative possible assurance to the slave, so we
        send them the earliest of the timestamps that the different slices' backfills gave
        us. */
        if (minimum_timestamp == repli_timestamp_t::invalid ||
                oper_start_timestamp.time < minimum_timestamp.time) {
            minimum_timestamp = oper_start_timestamp;
        }

        outstanding_backfills--;
        if (outstanding_backfills == 0) {
            /* We're done backfilling. */
            handler_->backfill_done(minimum_timestamp);

            /* This allows us to shut down */
            pulsed_when_backfill_over.pulse();
        }
    }

    /* Startup, shutdown, and member variables */

    btree_key_value_store_t *internal_store_;
    backfill_and_realtime_streaming_callback_t *handler_;

    realtime_mutation_dispatcher_t *realtime_mutation_dispatchers[MAX_SLICES];
    int outstanding_backfills;
    repli_timestamp_t minimum_timestamp;

    drain_semaphore_t time_barriers_drain_semaphore_;

    // If no actions come in, we periodically send time-barriers
    // anyway, to keep up a heartbeat.  This is the timer token for
    // the next time we must send a time-barrier (if we haven't already).
    timer_token_t *idle_timer_;

    // The latest timestamp we've seen.  Every time we get a new
    // timestamp, we update this value and spawn
    // note_timestamp(the_new_timestamp),
    // which tells the slices to check in.
    repli_timestamp latest_timestamp_;

    /* We can't stop until backfill is over and we can't interrupt a running backfill operation,
    so we have to use pulsed_when_backfill_over to wait for the backfill operation to finish. */
    cond_t pulsed_when_backfill_over;

    backfill_and_streaming_manager_t(btree_key_value_store_t *kvs,
            backfill_and_realtime_streaming_callback_t *handler,
            repli_timestamp_t timestamp) :
        internal_store_(kvs),
        handler_(handler),
        latest_timestamp_(current_time())
    {
        minimum_timestamp = repli_timestamp_t::invalid;

        /* On each thread, atomically start a backfill operation and start streaming
        realtime operations. If we start a backfill but don't start streaming operations
        immediately, operations will be lost; if we start streaming operations before we
        start a backfill, operations will be duplicated. Both are bad. */

        outstanding_backfills = 0;

        for (int i = 0; i < internal_store_->btree_static_config.n_slices; i++) {

            outstanding_backfills++;

            coro_t::spawn_on_thread(internal_store_->btrees[i]->home_thread,
                boost::bind(&backfill_and_streaming_manager_t::register_on_slice,
                    this,
                    i,
                    timestamp
                    ));
        }

        /* Start an initial idle timer */
        idle_timer_ = fire_timer_once(1100, &backfill_and_streaming_manager_t::idle_timer_callback, this);
    }

    void register_on_slice(int i, repli_timestamp_t timestamp) {

        /* We must register for realtime updates atomically together with starting the
        backfill operation. */

        btree_slice_t *slice = internal_store_->btrees[i];

        /* Register for real-time updates */
        realtime_mutation_dispatchers[i] = new realtime_mutation_dispatcher_t;
        realtime_mutation_dispatchers[i]->manager_ = this;
        slice->add_dispatcher(realtime_mutation_dispatchers[i]);

        /* Start a backfill operation */
        slice->backfill(timestamp, this);
    }

    ~backfill_and_streaming_manager_t() {

        /* We can't delete ourself until the backfill is over. It's impossible to interrupt
        a running backfill. The backfill logic takes care of calling handler_->backfill_done(). */
        pulsed_when_backfill_over.wait();

        /* Unregister for real-time updates */
        pmap(internal_store_->btree_static_config.n_slices,
            boost::bind(&backfill_and_streaming_manager_t::unregister_on_slice, this, _1));

        /* Cancel real-time time barrier idle timer */
        if (idle_timer_) cancel_timer(idle_timer_);

        /* Wait for any real-time time barriers to finish */
        time_barriers_drain_semaphore_.drain();

        /* Now we can stop */
    }

    void unregister_on_slice(int i) {

        btree_slice_t *slice = internal_store_->btrees[i];

        /* Unregister for realtime updates */
        slice->remove_dispatcher(realtime_mutation_dispatchers[i]);
        delete realtime_mutation_dispatchers[i];
    }
};

void backfill_and_realtime_stream(btree_key_value_store_t *kvs, repli_timestamp_t start_time,
        backfill_and_realtime_streaming_callback_t *bfh, multicond_t *pulse_to_stop) {

    {
        /* Constructing this object starts the backfill and the streaming. */
        backfill_and_streaming_manager_t manager(kvs, bfh, start_time);

        pulse_to_stop->wait();

        /* The destructor for "manager" waits for the backfill to finish, then stops the
        realtime stream. */
    }
}

}
