#include "clustering/immediate_consistency/branch/broadcaster.hpp"

#include "utils.hpp"
#include <boost/make_shared.hpp>

#include "concurrency/coro_fifo.hpp"
#include "containers/death_runner.hpp"
#include "containers/uuid.hpp"
#include "clustering/immediate_consistency/branch/listener.hpp"
#include "clustering/immediate_consistency/branch/multistore.hpp"
#include "rpc/mailbox/typed.hpp"

template <class protocol_t>
const int broadcaster_t<protocol_t>::MAX_OUTSTANDING_WRITES =
    listener_t<protocol_t>::MAX_OUTSTANDING_WRITES_FROM_BROADCASTER;

template <class protocol_t>
broadcaster_t<protocol_t>::write_callback_t::write_callback_t() : write(NULL) { }

template <class protocol_t>
broadcaster_t<protocol_t>::write_callback_t::~write_callback_t() {
    if (write) {
        rassert(write->callback == this);
        write->callback = NULL;
    }
}

template <class protocol_t>
broadcaster_t<protocol_t>::broadcaster_t(mailbox_manager_t *mm,
        branch_history_manager_t<protocol_t> *bhm,
        multistore_ptr_t<protocol_t> *initial_svs,
        perfmon_collection_t *parent_perfmon_collection,
        order_source_t *order_source,
        signal_t *interruptor) THROWS_ONLY(interrupted_exc_t)
    : mailbox_manager(mm),
      branch_id(generate_uuid()),
      branch_history_manager(bhm),
      registrar(mailbox_manager, this),
      broadcaster_collection(),
      broadcaster_membership(parent_perfmon_collection, &broadcaster_collection, "broadcaster")
{
    order_checkpoint.set_tagappend("broadcaster_t");

    /* Snapshot the starting point of the store; we'll need to record this
       and store it in the metadata. */
    scoped_ptr_t<fifo_enforcer_sink_t::exit_read_t> read_token;
    initial_svs->new_read_token(&read_token);

    region_map_t<protocol_t, version_range_t> origins = initial_svs->get_all_metainfos(order_source->check_in("broadcaster_t(read)").with_read_mode(), &read_token, interruptor);

    /* Determine what the first timestamp of the new branch will be */
    state_timestamp_t initial_timestamp = state_timestamp_t::zero();
    typedef region_map_t<protocol_t, version_range_t> version_map_t;
    for (typename version_map_t::const_iterator it =  origins.begin();
         it != origins.end();
         it++) {
        state_timestamp_t part_timestamp = it->second.latest.timestamp;
        if (part_timestamp > initial_timestamp) {
            initial_timestamp = part_timestamp;
        }
    }

    /* Make an entry for this branch in the branch history manager */
    {
        branch_birth_certificate_t<protocol_t> birth_certificate;
        birth_certificate.region = initial_svs->get_multistore_joined_region();
        birth_certificate.initial_timestamp = initial_timestamp;
        birth_certificate.origin = origins;

        branch_history_manager->create_branch(branch_id, birth_certificate, interruptor);
    }

    /* Reset the store metadata. We should do this after making the branch
       entry in the global metadata so that we aren't left in a state where
       the store has been marked as belonging to a branch for which no
       information exists. */
    scoped_ptr_t<fifo_enforcer_sink_t::exit_write_t> write_token;
    initial_svs->new_write_token(&write_token);
    initial_svs->set_all_metainfos(region_map_t<protocol_t, binary_blob_t>(initial_svs->get_multistore_joined_region(),
                                                                           binary_blob_t(version_range_t(version_t(branch_id, initial_timestamp)))),
                                   order_source->check_in("broadcaster_t(write)"),
                                   &write_token,
                                   interruptor);

    /* Set up the shards */
    shards.reset(new one_per_thread_t<shard_t>(this, initial_timestamp));

    /* Set `bootstrap_store` so that the initial listener can find it */
    bootstrap_svs = initial_svs;
}

template <class protocol_t>
branch_id_t broadcaster_t<protocol_t>::get_branch_id() const {
    return branch_id;
}

template <class protocol_t>
broadcaster_business_card_t<protocol_t> broadcaster_t<protocol_t>::get_business_card() {
    branch_history_t<protocol_t> branch_id_associated_branch_history;
    branch_history_manager->export_branch_history(branch_id, &branch_id_associated_branch_history);
    return broadcaster_business_card_t<protocol_t>(branch_id, branch_id_associated_branch_history, registrar.get_business_card());
}

template <class protocol_t>
multistore_ptr_t<protocol_t> *broadcaster_t<protocol_t>::release_bootstrap_svs_for_listener() {
    rassert(bootstrap_svs != NULL);
    multistore_ptr_t<protocol_t> *tmp = bootstrap_svs;
    bootstrap_svs = NULL;
    return tmp;
}

/* `incomplete_write_t` represents a write that has been sent to some nodes
   but not completed yet. */
template <class protocol_t>
class broadcaster_t<protocol_t>::incomplete_write_t : public home_thread_mixin_t {
public:
    incomplete_write_t(shard_t *p, typename protocol_t::write_t w, transition_timestamp_t ts, write_callback_t *cb) :
        write(w), timestamp(ts), callback(cb), parent(p), incomplete_count(0) { }

    const typename protocol_t::write_t write;
    const transition_timestamp_t timestamp;
    write_callback_t *callback;

private:
    friend class incomplete_write_ref_t;

    shard_t *parent;
    int incomplete_count;

    DISABLE_COPYING(incomplete_write_t);
};

/* We keep track of which `incomplete_write_t`s have been acked by all the
   nodes using `incomplete_write_ref_t`. When there are zero
   `incomplete_write_ref_t`s for a given `incomplete_write_t`, then it is no
   longer incomplete. */

// TODO: make this noncopyable.
template <class protocol_t>
class broadcaster_t<protocol_t>::incomplete_write_ref_t {
public:
    incomplete_write_ref_t() { }
    explicit incomplete_write_ref_t(const boost::shared_ptr<incomplete_write_t> &w) : write(w) {
        rassert(w);
        w->incomplete_count++;
    }
    incomplete_write_ref_t(const incomplete_write_ref_t &r) : write(r.write) {
        if (r.write) {
            r.write->incomplete_count++;
        }
    }
    ~incomplete_write_ref_t() {
        if (write) {
            write->incomplete_count--;
            if (write->incomplete_count == 0) {
                write->parent->end_write(write);
            }
        }
    }
    incomplete_write_ref_t &operator=(const incomplete_write_ref_t &r) {
        if (r.write) {
            r.write->incomplete_count++;
        }
        if (write) {
            write->incomplete_count--;
            if (write->incomplete_count == 0) {
                write->parent->end_write(write);
            }
        }
        write = r.write;
        return *this;
    }
    boost::shared_ptr<incomplete_write_t> get() {
        return write;
    }
private:
    boost::shared_ptr<incomplete_write_t> write;
};

template <class protocol_t>
class broadcaster_t<protocol_t>::shard_t : public home_thread_mixin_t {
public:
    shard_t(broadcaster_t *p, state_timestamp_t ts) :
        parent(p),
        current_timestamp(ts.plus_integer(get_thread_id()))
        { }

    typename protocol_t::read_response_t read(
            typename protocol_t::read_t read,
            fifo_enforcer_sink_t::exit_read_t *lock,
            order_token_t order_token,
            signal_t *interruptor) THROWS_ONLY(cannot_perform_exc_t, interrupted_exc_t) {
        assert_thread();
        order_token.assert_read_mode();

        shard_dispatchee_t *reader;
        auto_drainer_t::lock_t reader_lock;
        state_timestamp_t timestamp;

        {
            wait_interruptible(lock, interruptor);
            mutex_assertion_t::acq_t mutex_acq(&mutex);
            lock->end();

            pick_a_readable_dispatchee(&reader, &mutex_acq, &reader_lock);

            timestamp = current_timestamp;

            /* This is safe even if `interruptor` gets pulsed because
            `shard_dispatchee_t::send_read_to_listener()` doesn't check
            `interruptor` until after it sends the message. */
            reader->num_reads_since_last_write++;

            order_token = order_checkpoint.check_through(order_token);
        }

        try {
            wait_any_t interruptor2(reader_lock.get_drain_signal(), interruptor);
            return reader->send_read_to_listener(read,
                fifo_enforcer_read_token_t(timestamp),
                order_token, &interruptor2);
        } catch (interrupted_exc_t) {
            if (interruptor->is_pulsed()) {
                throw;
            } else {
                throw cannot_perform_query_exc_t("lost contact with mirror during read");
            }
        }
    }

    void spawn_write(
            typename protocol_t::write_t write,
            fifo_enforcer_sink_t::exit_write_t *lock,
            order_token_t order_token,
            write_callback_t *cb,
            signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        order_token.assert_write_mode();

        wait_interruptible(lock, interruptor);
        ASSERT_FINITE_CORO_WAITING;

        sanity_check();

        /* We have to be careful about the case where dispatchees are joining or
        leaving at the same time as we are doing the write. The way we handle
        this is via `mutex`. If the write reaches `mutex` before a new
        dispatchee does, then the new dispatchee's constructor will send off the
        write. Otherwise, the write will be sent directly to the new dispatchee
        by the loop further down in this very function. */
        mutex_assertion_t::acq_t mutex_acq(&mutex);

        lock->end();

        /* Once we modify `current_timestamp`, we are committed to sending the
        write to every dispatchee and we mustn't allow ourselves to be
        interrupted after this point. */
        transition_timestamp_t timestamp = transition_timestamp_t::starting_from(current_timestamp);
        current_timestamp = current_timestamp.plus_integer(get_num_threads());
        order_token = order_checkpoint.check_through(order_token);

        boost::shared_ptr<incomplete_write_t> write_wrapper = boost::make_shared<incomplete_write_t>(
            this, write, timestamp, cb);
        incomplete_writes.push_back(write_wrapper);

        rassert(cb->write == NULL, "You can't reuse the same callback for two writes.");
        cb->write = write_wrapper.get();

        /* Create a reference so that `write` doesn't declare itself
        complete before we've even started */
        incomplete_write_ref_t write_ref = incomplete_write_ref_t(write_wrapper);

        for (typename std::map<shard_dispatchee_t *, auto_drainer_t::lock_t>::iterator it = dispatchees.begin();
                it != dispatchees.end(); it++) {
            shard_dispatchee_t *dispatchee = it->first;
            fifo_enforcer_write_token_t fifo_enforcer_token(
                timestamp,
                dispatchee->num_reads_since_last_write);
            dispatchee->num_reads_since_last_write = 0;
            dispatchee->background_write_queue.push(boost::bind(
                &shard_dispatchee_t::background_write, dispatchee,
                write_ref, order_token, fifo_enforcer_token, it->second
                ));
        }
    }

    /* Reads need to pick a single readable mirror to perform the operation.
    Writes need to choose a readable mirror to get the reply from. Both use
    `pick_a_readable_dispatchee()` to do the picking. You must hold
    `mutex` and pass in `proof` of the mutex acquisition. (A dispatchee is
    "readable" if a `replier_t` exists for it on the remote machine.) */
    void pick_a_readable_dispatchee(dispatchee_t **dispatchee_out, mutex_assertion_t::acq_t *proof, auto_drainer_t::lock_t *lock_out) THROWS_ONLY(cannot_perform_query_exc_t) {
        ASSERT_FINITE_CORO_WAITING;
        proof->assert_is_holding(&mutex);

        if (readable_dispatchees.empty()) {
            throw cannot_perform_query_exc_t("no mirrors readable. this is strange because the primary mirror should be always readable.");
        }
        *dispatchee_out = readable_dispatchees.head();

        /* Cycle the readable dispatchees so that the load gets distributed
        evenly */
        readable_dispatchees.pop_front();
        readable_dispatchees.push_back(*dispatchee_out);

        *lock_out = dispatchees[*dispatchee_out];
    }

    void end_write(boost::shared_ptr<incomplete_write_t> write) THROWS_NOTHING {
        mutex_assertion_t::acq_t mutex_acq(&mutex);
        ASSERT_FINITE_CORO_WAITING;
        /* It's safe to remove a write from the queue once it has acquired the
        root of every listener's btree, because that guarantees that every
        future backfill from any listener will see it. We aren't notified when
        it acquires the root; we're notified when it finishes, which happens
        some unspecified amount of time after it acquires the root. When a given
        write has finished on every listener, then we know that it and every
        write before it have acquired the root, even though some of the writes
        before it might not have finished yet. So when a write is finished on
        every listener, we remove it and every write before it from the queue.
        This loop makes one iteration on average for every call to
        `end_write()`, but it could make multiple iterations or zero iterations
        in a single call. */
        while (!incomplete_writes.empty() &&
                incomplete_writes.front()->timestamp <= write->timestamp) {
            incomplete_writes.pop_front();
        }
        if (write->callback) {
            rassert(write->callback->write == write.get());
            write->callback->write = NULL;
            write->callback->on_done();
        }
    }

    void catch_up_to(state_timestamp_t state_timestamp) {
        
    }

    /* If a write has begun, but some listener might not have completed it yet,
    then it goes in `incomplete_writes`. The idea is that a new listener that
    connects will use the union of a backfill and `incomplete_writes` as its
    data, and that will guarantee it gets at least one copy of every write. */

    /* `mutex` is held by new writes and reads being created, by writes
    finishing, and by dispatchees joining, leaving, or upgrading. It protects
    `incomplete_writes`, `current_timestamp`, `order_checkpoint`, `dispatchees`,
    and `readable_dispatchees`. */
    mutex_assertion_t mutex;

    std::list<boost::shared_ptr<incomplete_write_t> > incomplete_writes;
    state_timestamp_t current_timestamp;
    order_checkpoint_t order_checkpoint;

    std::map<shard_dispatchee_t *, auto_drainer_t::lock_t> dispatchees;
    intrusive_list_t<shard_dispatchee_t> readable_dispatchees;
};

/* The `registrar_t` constructs a `dispatchee_t` for every mirror that
   connects to us. */

template <class protocol_t>
class broadcaster_t<protocol_t>::dispatchee_t {
public:
    dispatchee_t(broadcaster_t *b, listener_business_card_t<protocol_t> lbc) THROWS_NOTHING :
        broadcaster(b),
        upgrade_mailbox(broadcaster->mailbox_manager,
            boost::bind(&dispatchee_t::on_upgrade, this, _1, _2),
            mailbox_callback_mode_inline),
        downgrade_mailbox(broadcaster->mailbox_manager,
            boost::bind(&dispatchee_t::on_downgrade, this, _1),
            mailbox_callback_mode_inline)
    {
        broadcaster->assert_thread();

        std::vector<state_timestamp_t> initial_timestamps;
        initial_timestamps.resize(get_num_threads());

        shards.reset(new one_per_thread_t<shard_dispatchee_t>(this, lbc, &initial_timestamps));

        /* Because the different shards might not be exactly in sync, there may
        not exist a specific `state_timestamp_t` such that the listener can
        expect to receive a write for every transition timestamp after it and
        no write for any transition timestamp before it. We work around this by
        sending the earliest state timestamp for which the listener can expect
        to receive a write, and then sending "skip" commands to "fill in the
        holes" caused by some of the shards being faster than that shard. */
        state_timestamp_t earliest_initial_timestamp = initial_timestamps[0];
        for (int i = 1; i < get_num_threads(); i++) {
            earliest_initial_timestamp = std::min(earliest_initial_timestamp, initial_timestamps[i]);
        }
        send(broadcaster->mailbox_manager, lbc.intro_mailbox,
            earliest_initial_timestamp,
            upgrade_mailbox.get_address(),
            downgrade_mailbox.get_address()
            );
        for (int i = 0; i < get_num_threads(); i++) {
            transition_timestamp_t first_to_skip = transition_timestamp_t::starting_from(initial_timestamps[i]);
            int num_to_skip = 0;
            while (first_to_skip.plus_integer(-get_num_threads()).timestamp_before() >= earliest_initial_timestamp) {
                first_to_skip = first_to_skip.plus_integer(-get_num_threads());
                ++num_to_skip;
            }
            if (num_to_skip > 0) {
                send(broadcaster->mailbox_manager, lbc.skip_mailbox,
                    0, first_to_skip, num_to_skip, get_num_threads());
            }
        }
    }

    peer_id_t get_peer() {
        return write_mailbox.get_peer();
    }

    
private:
    /* `on_upgrade()` and `on_downgrade()` are mailbox callbacks. They spawn
    `perform_upgrade()` and `perform_downgrade()` in coroutines, which use
    `pmap()` to spawn many instances of `perform_{up,down}grade_on_thread()` in
    parallel, which call `{up,down}grade()` on the `shard_dispatchee_t` object.
    */

    void on_upgrade(typename listener_business_card_t<protocol_t>::writeread_mailbox_t::address_t wrm,
            typename listener_business_card_t<protocol_t>::read_mailbox_t::address_t rm) THROWS_NOTHING {
        coro_t::spawn_sometime(boost::bind(&dispatchee_t::perform_upgrade, this,
            wrm, rm, auto_drainer_t::lock_t(&drainer)));
    }

    void perform_upgrade(typename listener_business_card_t<protocol_t>::writeread_mailbox_t::address_t wrm,
            typename listener_business_card_t<protocol_t>::read_mailbox_t::address_t rm,
            auto_drainer_t::lock_t keepalive) THROWS_NOTHING {
        pmap(get_num_threads(), boost::bind(&dispatchee_t::perform_upgrade_on_thread, this, _1, wrm, rm));
    }

    void perform_upgrade_on_thread(int thread,
            typename listener_business_card_t<protocol_t>::writeread_mailbox_t::address_t wrm,
            typename listener_business_card_t<protocol_t>::read_mailbox_t::address_t rm) THROWS_NOTHING {
        on_thread_t thread_switcher(thread);
        shards->get()->upgrade(wrm, rm);
    }

    void on_downgrade(mailbox_addr_t<void()> ack_addr) THROWS_NOTHING {
        coro_t::spawn_sometime(boost::bind(&dispatchee_t::perform_downgrade, this,
            ack_addr, auto_drainer_t::lock_t(&drainer)));
    }

    void perform_downgrade(mailbox_addr_t<void()> ack_addr, auto_drainer_t::lock_t keepalive) THROWS_NOTHING {
        pmap(get_num_threads(), boost::bind(&dispatchee_t::perform_downgrade_on_thread, this, _1));
        send(broadcaster->mailbox_manager, ack_addr);
    }

    void perform_downgrade_on_thread(int thread) THROWS_NOTHING {
        on_thread_t thread_switcher(thread);
        shards->get()->downgrade();
    }

    broadcaster_t *broadcaster;

    scoped_ptr_t<one_per_thread_t<shard_dispatchee_t> > shards;

    auto_drainer_t drainer;

    typename listener_business_card_t<protocol_t>::upgrade_mailbox_t upgrade_mailbox;
    typename listener_business_card_t<protocol_t>::downgrade_mailbox_t downgrade_mailbox;
};

template <class protocol_t>
class broadcaster_t<protocol_t>::shard_dispatchee_t :
        public intrusive_list_node_t<shard_dispatchee_t>,
        public home_thread_mixin_t {
public:
    shard_dispatchee_t(broadcaster_t *b, listener_business_card_t<protocol_t> lbc, std::vector<state_timestamp_t> *initial_timestamps_out) :
        broadcaster(b), shard(broadcaster->shards->get()),
        write_mailbox(lbc.write_mailbox), skip_mailbox(lbc.skip_mailbox),
        is_readable(false),
        queue_count_membership(&c->broadcaster_collection, &queue_count,
            strprintf("%s_broadcast_queue_%d_count", uuid_to_str(d.write_mailbox.get_peer().get_uuid()), get_thread_id())),
        background_write_queue(&queue_count),
        /* TODO magic constant */
        background_write_workers(100, &background_write_queue, &background_write_caller),
        num_reads_since_last_write(0)
    {
        shard->assert_thread();
        shard->sanity_check();

        /* Grab mutex so we don't race with writes that are starting or
        finishing. If we don't do this, bad things could happen: for
        example, a write might get dispatched to us twice if it starts after
        we're in `shard->dispatchees` but before we've iterated over
        `incomplete_writes`. */
        mutex_assertion_t::acq_t acq(&shard->mutex);
        ASSERT_FINITE_CORO_WAITING;

        shard->dispatchees[this] = auto_drainer_t::lock_t(&drainer);

        initial_timestamps_out[get_thread_id()] = shard->newest_complete_timestamp;

        for (typename std::list<boost::shared_ptr<incomplete_write_t> >::iterator it = controller->incomplete_writes.begin();
                it != controller->incomplete_writes.end(); it++) {
            background_write_queue.push(boost::bind(&broadcaster_t::background_write, broadcaster,
                this, auto_drainer_t::lock_t(&drainer), incomplete_write_ref_t(*it), order_source.check_in("shard_dispatchee_t"), fifo_source.enter_write()));
        }
    }

    ~shard_dispatchee_t() THROWS_NOTHING {
        ASSERT_FINITE_CORO_WAITING;
        shard->assert_thread();
        mutex_assertion_t::acq_t acq(&shard->mutex);
        if (is_readable) shard->readable_dispatchees.remove(this);
        shard->dispatchees.erase(this);
    }

    void upgrade(typename listener_business_card_t<protocol_t>::writeread_mailbox_t::address_t wrm,
            typename listener_business_card_t<protocol_t>::read_mailbox_t::address_t rm) {
        ASSERT_FINITE_CORO_WAITING;
        mutex_assertion_t::acq_t acq(&shard->mutex);
        rassert(!is_readable);
        is_readable = true;
        writeread_mailbox = wrm;
        read_mailbox = rm;
        shard->readable_dispatchees.push_back(this);
    }

    void downgrade() {
        ASSERT_FINITE_CORO_WAITING;
        mutex_assertion_t::acq_t acq(&controller->mutex);
        rassert(is_readable);
        is_readable = false;
        shard->readable_dispatchees.remove(this);
    }

    void background_write(incomplete_write_ref_t write_ref,
            order_token_t order_token,
            fifo_enforcer_write_token_t token,
            auto_drainer_t::lock_t keepalive) THROWS_NOTHING {
        assert_thread();
        order_token.assert_write_mode();
        try {
            if (is_readable) {
                promise_t<typename protocol_t::write_response_t> resp_cond;
                mailbox_t<void(typename protocol_t::write_response_t)> resp_mailbox(
                    broadcaster->mailbox_manager,
                    boost::bind(&promise_t<typename protocol_t::write_response_t>::pulse, &resp_cond, _1),
                    mailbox_callback_mode_inline);
                send(broadcaster->mailbox_manager, writeread_mailbox,
                    write_ref.get()->write, fifo_token, order_token, resp_mailbox.get_address());
                wait_interruptible(resp_cond.get_ready_signal(), keepalive.get_drain_signal());
                if (write_ref.get()->callback) {
                    write_ref.get()->callback->on_response(get_peer(), resp_cond.get_value());
                }
            } else {
                cond_t ack_cond;
                mailbox_t<void()> ack_mailbox(
                    broadcaster->mailbox_manager,
                    boost::bind(&cond_t::pulse, &ack_cond),
                    mailbox_callback_mode_inline);
                send(broadcaster->mailbox_manager, write_mailbox,
                    write_ref.get()->write, fifo_token, order_token, ack_mailbox.get_address());
                wait_interruptible(&ack_cond, interruptor);
            }
        } catch (interrupted_exc_t) {
            /* we're shutting down, so nothing matters */
        }
    }

    void send_read_to_listener(
            typename protocol_t::read_t read,
            fifo_enforcer_read_token_t fifo_token,
            order_token_t order_token,
            signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        assert_thread();
        order_token.assert_read_mode();
        promise_t<typename protocol_t::read_response_t> resp_cond;
        mailbox_t<void(typename protocol_t::read_response_t)> resp_mailbox(
            broadcaster->mailbox_manager,
            boost::bind(&promise_t<typename protocol_t::read_response_t>::pulse, &resp_cond, _1),
            mailbox_callback_mode_inline);
        send(broadcaster->mailbox_manager, read_mailbox,
            read, fifo_token, order_token, resp_mailbox.get_address());
        wait_interruptible(resp_cond.get_ready_signal(), interruptor);
        return resp_cond.get_value();
    }

    void send_skip_to_listener(
            int64_t num_reads_before_first_to_skip,
            transition_timestamp_t first_to_skip,
            int how_many_to_skip, int interval) THROWS_NOTHING {
        assert_thread();
        send(broadcaster->mailbox_manager, skip_mailbox,
            num_reads_before_first_to_skip, first_to_skip, how_many_to_skip, interval);
    }

    broadcaster_t *broadcaster;
    shard_t *shard;

    typename listener_business_card_t<protocol_t>::write_mailbox_t::address_t write_mailbox;
    typename listener_business_card_t<protocol_t>::writeread_mailbox_t::address_t writeread_mailbox;
    typename listener_business_card_t<protocol_t>::read_mailbox_t::address_t read_mailbox;
    typename listener_business_card_t<protocol_t>::skip_mailbox_t::address_t skip_mailbox;
    bool is_readable;

    order_source_t order_source;

    perfmon_counter_t queue_count;
    perfmon_membership_t queue_count_membership;
    unlimited_fifo_queue_t<boost::function<void()> > background_write_queue;
    calling_callback_t background_write_caller;
    coro_pool_t<boost::function<void()> > background_write_workers;

    /* `shard_t` uses this to compute the `num_preceding_reads` field of each
    `fifo_enforcer_write_token_t` */
    int64_t num_reads_since_last_write;

    auto_drainer_t drainer;
};

template<class protocol_t>
typename protocol_t::read_response_t broadcaster_t<protocol_t>::read(typename protocol_t::read_t read, fifo_enforcer_sink_t::exit_read_t *lock, order_token_t order_token, signal_t *interruptor) THROWS_ONLY(cannot_perform_query_exc_t, interrupted_exc_t) {
    shards.get()->read(read, lock, order_token, interruptor);
}

template<class protocol_t>
void broadcaster_t<protocol_t>::spawn_write(typename protocol_t::write_t write, fifo_enforcer_sink_t::exit_write_t *lock, order_token_t order_token, write_callback_t *cb, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    shards.get()->spawn_write(write, lock, order_token, cb, interruptor);
}

template <class protocol_t>
void broadcaster_t<protocol_t>::sync_until_shutdown(auto_drainer_t::lock_t keepalive) {
    state_timestamp_t timestamp = state_timestamp_t::zero();
    while (!keepalive.get_drain_signal()->is_pulsed()) {
        std::vector<state_timestamp_t> current_timestamps;
        current_timestamps.resize(get_num_threads());
        pmap(get_num_threads(), boost::bind(&broadcaster_t::sync_on_thread, timestamp, current_timestamps.data()));
        for (int i = 0; i < get_num_threads(); i++) {
            timestamp = std::max(timestamp, current_timestamps[i]);
        }
    }
}

template <class protocol_t>
void sync_on_thread(int thread, state_timestamp_t go_at_least_to, state_timestamp_t *current_out) {
    on_thread_t thread_switcher(thread);
    
}


#include "memcached/protocol.hpp"
#include "mock/dummy_protocol.hpp"
#include "rdb_protocol/protocol.hpp"

template class broadcaster_t<mock::dummy_protocol_t>;
template class broadcaster_t<memcached_protocol_t>;
template class broadcaster_t<rdb_protocol_t>;
